$$
\huge \displaystyle \dot{x} = f(x,u)
$$

---

Drone models @ LSY. Contains symbolic (CasADi) and numeric (array API, i.e., NumPy, JAX, ...) drone dynamics.

[![Python Version]][Python Version URL] [![Ruff Check]][Ruff Check URL] [![Tests]][Tests URL] [![Docs]][Docs URL]

[Python Version]: https://img.shields.io/badge/python-3.10+-blue.svg
[Python Version URL]: https://www.python.org

[Ruff Check]: https://github.com/utiasDSL/drone-models/actions/workflows/ruff.yml/badge.svg?style=flat-square
[Ruff Check URL]: https://github.com/utiasDSL/drone-models/actions/workflows/ruff.yml

[Tests]: https://github.com/utiasDSL/drone-models/actions/workflows/testing.yml/badge.svg
[Tests URL]: https://github.com/utiasDSL/drone-models/actions/workflows/testing.yml

[Docs]: https://github.com/utiasDSL/drone-models/actions/workflows/docs.yml/badge.svg
[Docs URL]: https://utiasdsl.github.io/drone-models/

## Overview
Collection of different drone dynamics models. Also includes meshes of each model.

## Installation

1. Clone repository `git clone git@github.com:utiasDSL/drone-models.git`
2. Enter repository `cd drone-models`
3. Install locally with `pip install -e .` or the pixi environment with `pixi install`, which can be activated with `pixi shell`


## Usage
`from drone_models import TODO`


## Testing
1. Install testing environment with `pixi install -e test`
1. Run tests with `pixi run -e test pytest`
