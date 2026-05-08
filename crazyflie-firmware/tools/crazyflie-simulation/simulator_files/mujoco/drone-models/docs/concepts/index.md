# Concepts

Understanding the fundamental concepts behind Drone Models will help you use the package effectively and choose the right approach for your application.

## Core Design Principles

### Functional Programming Paradigm

All models in this package are **pure functions** - they have no internal state and produce deterministic outputs based solely on their inputs. This design choice offers several advantages:

- **Predictability**: The same inputs always produce the same outputs
- **Parallelizability**: Functions can be safely called in parallel
- **Composability**: Easy to combine and integrate with other systems
- **Testability**: Straightforward to unit test and validate

### Array API Compatibility

Models are built using the [Array API standard](https://data-apis.org/array-api/latest/), enabling seamless backend switching without code changes:

```python
import numpy as np
import jax.numpy as jnp
import torch

# Same model, different backends
model_numpy = parametrize(dynamics, "cf2x_L250", xp=np)
model_jax = parametrize(dynamics, "cf2x_L250", xp=jnp)
model_torch = parametrize(dynamics, "cf2x_L250", xp=torch)
```

### Batching by Default

All models support batch processing out of the box, allowing you to simulate multiple drones or scenarios simultaneously:

```python
# Single drone
pos = np.array([0., 0., 1.])           # Shape: (3,)

# 100 drones
pos_batch = np.zeros((100, 3))         # Shape: (100, 3)

# 10x5 grid of scenarios  
pos_grid = np.zeros((10, 5, 3))        # Shape: (10, 5, 3)
```

## Key Concepts

### **[Drone Models](drone-models.md)**
Learn about different model types and their trade-offs:
- Physics-based vs. data-driven approaches
- Model complexity levels
- When to use each model type

### **[Parametrization](parameters.md)**
Understand how model parameters work:
- Parameter files and loading
- Default configurations for common drones
- Custom parameter definition

### **[Transformations](transformations.md)**
Explore coordinate and parameter transformations:
- Motor forces to rotor velocities
- PWM to force conversions
- Body frame transformations

## Advanced Topics

### Symbolic vs. Numeric Models

Each model type offers both numeric and symbolic implementations:

- **Numeric models**: Fast execution, suitable for simulation and real-time control
- **Symbolic models**: Generate mathematical expressions for optimization solvers (CasADi)

```python
# Numeric model for simulation
from drone_models.first_principles import dynamics

# Symbolic model for MPC
from drone_models.first_principles import symbolic_dynamics
```

### JIT Compilation

Models are designed to work seamlessly with JAX's JIT compiler for maximum performance:

```python
import jax

# JIT compile for speed
model_jit = jax.jit(model)
derivatives = model_jit(pos, quat, vel, ang_vel, cmd)
```

### GPU Acceleration

Thanks to Array API compatibility, models automatically support GPU acceleration when using appropriate backends:

```python
import jax
import jax.numpy as jnp

# Move computation to GPU
pos = jnp.array(pos_data, device=jax.devices("gpu")[0])
model_gpu = parametrize(dynamics, "cf2x_L250", xp=jnp)
```

## Mathematical Foundations

All models are based on well-established principles of rigid body dynamics and aerodynamics. The mathematical details are covered in each model's specific documentation, but the general approach follows:

1. **State Representation**: Position, orientation (quaternion), linear and angular velocities
2. **Input Mapping**: Motor commands to forces and torques
3. **Dynamics Computation**: Newton-Euler equations for rigid body motion
4. **Optional Extensions**: Rotor dynamics, aerodynamic effects, disturbances

## Next Steps

- **New to drone modeling?** Start with [Drone Models](drone-models.md) to understand the basics
- **Ready to use the package?** Check out [Parameters](parameters.md) for configuration
- **Need coordinate conversions?** Explore [Transformations](transformations.md)

---

*The functional design and Array API compatibility make Drone Models uniquely suited for modern scientific computing workflows.*
