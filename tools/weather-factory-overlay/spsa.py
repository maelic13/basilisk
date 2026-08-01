"""Weather-factory SPSA core — Basilisk overlay.  BASILISK_SPSA_SCHEDULE_V1

Upstream: https://github.com/jnlt3/weather-factory (MIT, Copyright (c) 2024
jnlt3).  This file REPLACES the clone's ``spsa.py``.  ``tools/weather-factory/``
is gitignored, so anything not kept here is lost on the next re-clone;
``tools/setup_tools.ps1`` copies this file in and ``tools/spsa.ps1`` refuses to
set up or launch a tune unless the clone's copy matches this one byte for byte.

Basilisk changes (Phase 9.1, 2026-07-29 — PLAN.md §5 step 9.1):

1. **Schedule units.**  Upstream advances ``self.t`` by ``cutechess.games``
   (32 games per iteration) and then feeds that *game* count straight into both
   schedule terms, while ``spsa.ps1`` writes ``A`` in *iterations*.  ``A`` was
   therefore effectively 1/32 of its intended value — the "damp the first 10% of
   the run" term damped essentially nothing — and the gain decayed ~8x faster
   than the design implied.  Every Basilisk SPSA ever run, ``hcefinal``
   included, annealed far too fast.  ``self.t`` still counts games so existing
   ``tuner/state.json`` files resume unchanged; the schedule converts to
   iterations first.

2. **End-state parameterization.**  ``SpsaParams.from_end_state()`` back-solves
   ``a``/``c``/``A`` from the planned horizon ``N`` and the end-of-run step
   ratio ``r_end`` (fishtest's parameterization).  Shipping change 1 *without*
   this multiplies every step by ~8, which is worse than the bug: for
   ``k >> A/32`` the old games-fed decay was the right shape times a constant.
   Deriving both constants from ``N`` also means changing the horizon can no
   longer silently change end behaviour, and ``a`` can never be left stale.

3. **``N`` and ``r_end`` are recorded in the params** so a resumed run can
   report progress against the planned horizon and state loudly that ``A`` was
   frozen at first launch.

The schedule maths lives in this one file — ``write_spsa_json.py`` (which
``spsa.ps1`` calls to emit ``spsa.json``) and
``tools/verify_spsa_schedule.py`` both import it rather than re-deriving the
formulas.
"""

from __future__ import annotations

from dataclasses import dataclass
from random import randint
from typing import TYPE_CHECKING
import copy

if TYPE_CHECKING:  # pragma: no cover
    # Import-time-only in the tuner; deferred so that the schedule maths in this
    # file can be imported (and verified) without weather-factory's runtime.
    # tools/verify_spsa_schedule.py does exactly that.
    from cutechess import CutechessMan


@dataclass
class Param:
    name: str
    value: float
    min_value: int
    max_value: int
    step: float

    def __post_init__(self):
        self.start_val: float = self.value
        assert self.step > 0

    def get(self) -> int:
        return round(self.value)

    def update(self, amt: float):
        self.value = min(max(self.value + amt, self.min_value), self.max_value)

    @property
    def as_uci(self) -> str:
        return f"option.{self.name}={self.get()}"

    def get_change(self) -> str:
        if self.value > self.start_val:
            return f"+{self.value - self.start_val:.2f}"
        elif self.value < self.start_val:
            return f"-{self.start_val - self.value:2f}"
        else:
            return f"+-0"

    def __str__(self) -> str:
        return (
            f"{self.name} = {self.get()}({self.get_change()}) in "
            f"[{self.min_value}, {self.max_value}] "
        )


@dataclass
class SpsaParams:
    a: float
    c: float
    A: float
    alpha: float = 0.601
    gamma: float = 0.102
    # Basilisk additions.  Both default so a pre-9.1 tuner/state.json still
    # loads: SpsaParams(**state["spsa_params"]) simply gets N = r_end = 0.
    N: int = 0          # planned horizon, in ITERATIONS (0 = not recorded)
    r_end: float = 0.0  # end-of-run step ratio this schedule was solved for

    def schedule(self, iteration: float) -> tuple[float, float]:
        """(a_t, c_t) at `iteration`, which is counted in ITERATIONS (1-based).

        This is the single definition of the schedule; `SpsaTuner.step`, the
        json writer and the verification script all read it from here.
        """
        a_t = self.a / (iteration + self.A) ** self.alpha
        c_t = self.c / iteration ** self.gamma
        return a_t, c_t

    def step_ratio(self, iteration: float | None = None) -> float:
        """The realised r_end at `iteration` (default: the planned horizon).

        A parameter perturbed by `param.step * c_t` is moved by
        `a_t * param.step / c_t` per unit of match signal, so `a_t / c_t` is
        exactly fishtest's `r_end` expressed in units of `param.step`.  Reading
        this at N is how the verifier checks that r_end is horizon-invariant.
        """
        if iteration is None:
            iteration = self.N
        a_t, c_t = self.schedule(iteration)
        return a_t / c_t

    @classmethod
    def from_end_state(
        cls,
        n_iterations: int,
        r_end: float,
        alpha: float = 0.601,
        gamma: float = 0.102,
    ) -> "SpsaParams":
        """Back-solve (a, c, A) from the planned horizon and the end step ratio.

        Fishtest states an SPSA tune by its *end* behaviour, per parameter:

            c_i = c_end_i * N**gamma
            a_i = (r_end * c_end_i**2) * (A + N)**alpha
            A   = 0.1 * N

        Weather-factory factors the same thing differently: it keeps ONE global
        `a` and `c` and scales every parameter by its own `param.step`, so that
        the perturbation is `param.step * c_t` and the update per unit of match
        signal is `a_t * param.step / c_t`.  Mapping `param.step` onto fishtest's
        per-parameter `c_end_i` (which is what it already means — the size of the
        probe for that knob) and equating the two update magnitudes at every
        iteration k gives, with c_end_i cancelling out:

            c = N**gamma           (so the probe equals param.step at k = N)
            a = r_end * (A + N)**alpha

        The equality is exact for all k, not just at the horizon.  `r_end` is
        therefore "how far one unit of match signal moves a parameter, in units
        of that parameter's own step, at the end of the run".

        Default `r_end = 0.0031` (see spsa.ps1): fishtest's own default is
        ~0.002, and it is also where our historical `a = 1.0` lands once the
        units are right at a ~1,000-iteration horizon divided by ten — matching
        the a ~ 0.1 that Rarog's trajectory-validated simulation preferred
        (RMSE 0.32 at a~0.1 vs 0.78 at the un-rescaled a = 1.0).
        """
        if n_iterations < 1:
            raise ValueError("n_iterations must be >= 1")
        if r_end <= 0:
            raise ValueError("r_end must be > 0")
        A = max(1.0, 0.1 * n_iterations)
        c = float(n_iterations) ** gamma
        a = r_end * (A + n_iterations) ** alpha
        return cls(a=a, c=c, A=A, alpha=alpha, gamma=gamma,
                   N=int(n_iterations), r_end=r_end)


class SpsaTuner:

    def __init__(
        self,
        spsa_params: SpsaParams,
        uci_params: list[Param],
        cutechess: CutechessMan
    ):
        self.uci_params = uci_params
        self.spsa = spsa_params
        self.cutechess = cutechess
        self.delta = [0] * len(uci_params)
        self.t = 0

    @property
    def iteration(self) -> float:
        """Completed iterations.  `self.t` is kept in GAMES for state compat."""
        return self.t / self.cutechess.games

    def step(self):
        self.t += self.cutechess.games
        # BASILISK_SPSA_SCHEDULE_V1: the schedule is defined in iterations, and
        # `A` is written in iterations, so `t` (games) must be converted here.
        # Upstream passed `t` straight in, making A ~32x too small in effect.
        a_t, c_t = self.spsa.schedule(self.iteration)

        self.delta = [randint(0, 1) * 2 - 1 for _ in range(len(self.delta))]

        uci_params_a = []
        uci_params_b = []
        for param, delta in zip(self.uci_params, self.delta):
            curr_delta = param.step

            step = delta * curr_delta * c_t

            uci_a = copy.deepcopy(param)
            uci_b = copy.deepcopy(param)

            uci_a.update(step)
            uci_b.update(-step)

            uci_params_a.append(uci_a)
            uci_params_b.append(uci_b)

        gradient = self.gradient(uci_params_a, uci_params_b)

        for param, delta in zip(self.uci_params, self.delta):
            param_grad = gradient / (delta * c_t)
            param.update(-param_grad * a_t * param.step)

    @property
    def params(self) -> list[Param]:
        return self.uci_params

    def gradient(self, params_a: list[Param], params_b: list[Param]) -> float:
        str_params_a = [p.as_uci for p in params_a]
        str_params_b = [p.as_uci for p in params_b]
        game_result = self.cutechess.run(str_params_a, str_params_b)
        return (game_result.l - game_result.w)
