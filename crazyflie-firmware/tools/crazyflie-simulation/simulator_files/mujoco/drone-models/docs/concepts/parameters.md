# Parameters

Model parameters define the physical properties and configuration of your drone. Understanding how to work with parameters is essential for accurate modeling and simulation.

## Parameter System Overview

The Drone Models package uses a flexible parameter system that:

- **Separates physics from implementation**: Parameters are defined independently of model code
- **Supports multiple drone types**: Pre-configured parameters for common platforms
- **Enables batch operations**: Parameters can be vectorized for multiple drones
- **Provides type safety**: Parameter validation and error checking

## The Parametrize Function

The core of the parameter system is the `parametrize` function, which binds model parameters to model functions:

```python
from drone_models.core import parametrize
from drone_models.first_principles import dynamics

# Create a parametrized model for Crazyflie 2.x with 250mm props
model = parametrize(dynamics, drone_model="cf2x_L250")

# Now the model function has all parameters bound
derivatives = model(pos, quat, vel, ang_vel, cmd, rotor_vel)
```

## Available Drone Configurations

### Crazyflie 2.x Series

Pre-configured parameters are available for the popular Crazyflie 2.x quadrotor:

| Configuration | Description | Propeller Size | Use Case |
|---------------|-------------|----------------|----------|
| `cf2x_L250` | Standard configuration | 65mm | Indoor flight, research |
| `cf2x_P250` | Performance variant | 65mm | Agile flight |  
| `cf2x_T350` | Thrust-optimized | 65mm | Heavy payload |

```python
# Load different Crazyflie configurations
cf_standard = parametrize(dynamics, "cf2x_L250")
cf_performance = parametrize(dynamics, "cf2x_P250") 
cf_thrust = parametrize(dynamics, "cf2x_T350")
```

## Parameter Categories

### Physical Properties

**Mass and Inertia:**
```python
mass: float           # Drone mass [kg]
J: Array              # Inertia matrix [kg⋅m²] 
J_inv: Array          # Inverse inertia matrix [1/(kg⋅m²)]
```

**Geometric Properties:**
```python
L: float              # Motor arm length [m]
mixing_matrix: Array  # Motor rotation directions and positions
```

### Motor and Propulsion

**Motor Constants:**
```python
KF: float             # Thrust coefficient [N⋅s²/rad²]
KM: float             # Torque coefficient [N⋅m⋅s²/rad²]
thrust_tau: float     # Motor time constant [s]
```

**Command Mapping:**
```python
thrust_max: float     # Maximum thrust per motor [N]
pwm_max: float        # Maximum PWM value
```

### Environmental

```python
gravity_vec: Array    # Gravity vector [m/s²], typically [0, 0, -9.81]
```

## Backend Compatibility

Parameters automatically adapt to your chosen array backend:

```python
import numpy as np
import jax.numpy as jnp

# NumPy backend
model_np = parametrize(dynamics, "cf2x_L250", xp=np)

# JAX backend - parameters converted to JAX arrays
model_jax = parametrize(dynamics, "cf2x_L250", xp=jnp)

# GPU acceleration with JAX
model_gpu = parametrize(dynamics, "cf2x_L250", xp=jnp, 
                       device=jax.devices("gpu")[0])
```

## Batch Parameters

For simulating multiple drones with different parameters:

```python
import numpy as np

# Create base model
model = parametrize(dynamics, "cf2x_L250", xp=np)

# Modify parameters for batch of 10 drones with different masses
batch_size = 10
masses = np.linspace(0.025, 0.035, batch_size)  # Vary mass by ±20%

# Update the model parameters
model.keywords['mass'] = masses
```

## Custom Parameter Sets

### Loading from Files

Parameter files are stored in TOML format for easy editing:

```toml
# custom_drone.toml
[physical]
mass = 0.030
L = 0.046

[motor]
KF = 3.16e-10
KM = 7.94e-12
thrust_tau = 0.15

[inertia]
Ixx = 1.4e-5
Iyy = 1.4e-5  
Izz = 2.17e-5
```

### Creating Custom Parameters

You can define custom parameter sets by implementing the `ModelParams` protocol:

```python
from typing import NamedTuple
from drone_models.core import ModelParams

class CustomDroneParams(NamedTuple, ModelParams):
    mass: float
    L: float
    KF: float
    # ... other parameters
    
    @staticmethod
    def load(drone_model: str) -> 'CustomDroneParams':
        # Load from your custom source
        return CustomDroneParams(mass=0.030, L=0.046, ...)
```

## Parameter Validation

The parameter system includes validation to catch common errors:

```python
# Automatic validation
try:
    model = parametrize(dynamics, "invalid_drone")
except ValueError as e:
    print(f"Invalid drone model: {e}")

# Parameter consistency checks
try:
    derivatives = model(pos, quat, vel, ang_vel, negative_thrust_cmd)
except ValueError as e:
    print(f"Invalid inputs: {e}")
```

## Working with Parameters

### Inspecting Parameters

```python
# View all parameters in a model
model = parametrize(dynamics, "cf2x_L250")
print("Model parameters:")
for name, value in model.keywords.items():
    print(f"  {name}: {value}")
```

### Modifying Parameters

```python
# Create base model
model = parametrize(dynamics, "cf2x_L250")

# Modify specific parameters
model.keywords['mass'] = 0.035  # Heavier drone
model.keywords['KF'] = 3.5e-10  # Different propellers

# Use modified model
derivatives = model(pos, quat, vel, ang_vel, cmd)
```

### Parameter Sensitivity Analysis

```python
# Test parameter sensitivity
base_model = parametrize(dynamics, "cf2x_L250")
mass_variations = np.linspace(0.8, 1.2, 10)  # ±20% variation

results = []
for factor in mass_variations:
    model = parametrize(dynamics, "cf2x_L250")
    model.keywords['mass'] *= factor
    result = model(pos, quat, vel, ang_vel, cmd)
    results.append(result)
```

## Best Practices

### Parameter Organization
- Use descriptive drone model names (`cf2x_L250` not `drone1`)
- Keep parameter files version controlled
- Document parameter sources and measurement methods

### Performance Optimization
- Avoid frequent parameter changes in tight loops
- Use batch parameters for multiple drones
- Pre-compile with JAX when using varying parameters

### Validation
- Always validate parameters against physical limits
- Test edge cases (zero mass, negative thrust, etc.)
- Compare results with known reference data

---

*The parameter system provides the flexibility to model any quadrotor while maintaining type safety and performance.*
