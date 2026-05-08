# Transformations

The `transform` module provides functions for converting between different parameter spaces and coordinate systems commonly used in drone modeling and control.

## Overview

Transformations are essential when working with drone systems because:

- **Hardware interfaces** often use different units (PWM vs. thrust)
- **Control algorithms** may work in different coordinate frames
- **Sensors and actuators** have different representations
- **Optimization** requires specific input formats

All transformation functions are:
- **Pure functions**: No side effects, deterministic outputs
- **Array API compatible**: Work with NumPy, JAX, PyTorch, etc.
- **Batch-enabled**: Process multiple values simultaneously
- **Vectorized**: Efficient for large-scale operations

## Motor and Rotor Transformations

### Motor Force to Rotor Velocity

Convert from motor forces (Newtons) to rotor angular velocities (rad/s):

```python
from drone_models.transform import motor_force2rotor_vel

# Single motor
force = 0.5  # [N]
kf = 3.16e-10  # Thrust coefficient [N⋅s²/rad²]
rotor_vel = motor_force2rotor_vel(force, kf)

# Multiple motors
forces = np.array([0.5, 0.6, 0.4, 0.55])  # [N]
rotor_vels = motor_force2rotor_vel(forces, kf)

# Batch processing
forces_batch = np.random.rand(100, 4) * 1.0  # 100 drones, 4 motors each
rotor_vels_batch = motor_force2rotor_vel(forces_batch, kf)
```

**Mathematical relationship:**
$$F = k_f \omega^2 \implies \omega = \sqrt{\frac{F}{k_f}}$$

### Rotor Velocity to Body Forces

Convert rotor velocities to total body force in the drone's frame:

```python
from drone_models.transform import rotor_vel2body_force

rotor_vels = np.array([1000, 1100, 950, 1050])  # [rad/s]
kf = 3.16e-10

# Returns force vector [Fx, Fy, Fz] in body frame
body_force = rotor_vel2body_force(rotor_vels, kf)
print(body_force)  # [0, 0, total_thrust] for standard quadrotor
```

**Key points:**
- Assumes standard quadrotor configuration (thrust only in z-direction)
- Total thrust is sum of individual motor thrusts
- Returns 3D force vector in body frame

### Rotor Velocity to Body Torques

Convert rotor velocities to torques about the body frame axes:

```python
from drone_models.transform import rotor_vel2body_torque

rotor_vels = np.array([1000, 1100, 950, 1050])  # [rad/s]
kf = 3.16e-10  # Thrust coefficient
km = 7.94e-12  # Torque coefficient  
L = 0.046      # Arm length [m]

# Mixing matrix for standard quadrotor (+ configuration)
mixing_matrix = np.array([
    [ 1,  1,  1],  # Motor 1: +x, +y, +torque
    [-1, -1,  1],  # Motor 2: -x, -y, +torque  
    [-1,  1, -1],  # Motor 3: -x, +y, -torque
    [ 1, -1, -1],  # Motor 4: +x, -y, -torque
])

body_torque = rotor_vel2body_torque(rotor_vels, kf, km, L, mixing_matrix)
print(body_torque)  # [τx, τy, τz] in body frame
```

**Physical interpretation:**
- **τx (roll)**: Torque about x-axis from thrust differential
- **τy (pitch)**: Torque about y-axis from thrust differential  
- **τz (yaw)**: Net torque from motor drag and rotation direction

## PWM and Force Conversions

### Force to PWM

Convert desired thrust to PWM commands for motor controllers:

```python
from drone_models.transform import force2pwm

thrust = 0.6        # Desired thrust [N]
thrust_max = 1.0    # Maximum motor thrust [N]
pwm_max = 65535     # Maximum PWM value (16-bit)

pwm_command = force2pwm(thrust, thrust_max, pwm_max)
print(f"PWM: {pwm_command}")  # 39321 (60% of max)
```

### PWM to Force

Convert PWM commands back to actual thrust values:

```python
from drone_models.transform import pwm2force

pwm_command = 39321
thrust_actual = pwm2force(pwm_command, thrust_max, pwm_max)
print(f"Thrust: {thrust_actual} N")  # 0.6 N
```

**Use cases:**
- **Hardware interfacing**: Convert between control algorithms and motor drivers
- **Calibration**: Map PWM values to measured thrust
- **Simulation**: Model realistic actuator limitations

## Array API Compatibility

All transformations work seamlessly with different array backends:

```python
import numpy as np
import jax.numpy as jnp
import torch

# NumPy arrays
forces_np = np.array([0.5, 0.6, 0.4, 0.55])
result_np = motor_force2rotor_vel(forces_np, kf)

# JAX arrays  
forces_jax = jnp.array([0.5, 0.6, 0.4, 0.55])
result_jax = motor_force2rotor_vel(forces_jax, kf)

# PyTorch tensors
forces_torch = torch.tensor([0.5, 0.6, 0.4, 0.55])
result_torch = motor_force2rotor_vel(forces_torch, kf)
```

## Batch Processing

Transformations are optimized for batch operations:

```python
# Process 1000 drones simultaneously
batch_size = 1000
forces = np.random.rand(batch_size, 4) * 1.0  # Random forces

# Single function call processes entire batch
rotor_vels = motor_force2rotor_vel(forces, kf)
print(rotor_vels.shape)  # (1000, 4)

# Works with arbitrary batch dimensions
forces_2d = np.random.rand(50, 20, 4)  # 50x20 grid of drones
rotor_vels_2d = motor_force2rotor_vel(forces_2d, kf)
print(rotor_vels_2d.shape)  # (50, 20, 4)
```

## Common Transformation Pipelines

### Control to Simulation Pipeline

```python
# Typical control-to-simulation transformation chain
def control_to_dynamics(thrust_commands, drone_params):
    """Convert high-level thrust commands to simulation inputs."""
    
    # 1. Thrust commands to rotor velocities
    rotor_vels = motor_force2rotor_vel(thrust_commands, drone_params.kf)
    
    # 2. Rotor velocities to body forces and torques  
    body_force = rotor_vel2body_force(rotor_vels, drone_params.kf)
    body_torque = rotor_vel2body_torque(
        rotor_vels, drone_params.kf, drone_params.km, 
        drone_params.L, drone_params.mixing_matrix
    )
    
    return body_force, body_torque
```

### Hardware Interface Pipeline

```python
def control_to_hardware(thrust_commands, motor_params):
    """Convert thrust commands to PWM for hardware."""
    
    # Clamp to physical limits
    thrust_clamped = np.clip(thrust_commands, 0, motor_params.thrust_max)
    
    # Convert to PWM
    pwm_commands = force2pwm(
        thrust_clamped, motor_params.thrust_max, motor_params.pwm_max
    )
    
    # Ensure integer PWM values
    return pwm_commands.astype(np.uint16)
```

## Error Handling and Edge Cases

Transformation functions handle common edge cases gracefully:

```python
# Zero thrust case
zero_thrust = np.array([0.0, 0.0, 0.0, 0.0])
rotor_vels = motor_force2rotor_vel(zero_thrust, kf)
print(rotor_vels)  # [0, 0, 0, 0] - no division by zero

# Negative thrust (physically impossible)
negative_thrust = np.array([-0.1, 0.5, 0.3, 0.4])
# Functions may warn or clamp negative values
```

## Performance Considerations

### JIT Compilation

Transformations work seamlessly with JAX JIT compilation:

```python
import jax

@jax.jit
def fast_transform(forces, kf):
    return motor_force2rotor_vel(forces, kf)

# First call compiles, subsequent calls are fast
result = fast_transform(forces, kf)
```

### Memory Efficiency

Functions avoid unnecessary memory allocations:

```python
# In-place operations where possible
forces = np.array([0.5, 0.6, 0.4, 0.55])
rotor_vels = motor_force2rotor_vel(forces, kf)
# Original 'forces' array unchanged, minimal memory usage
```

## Integration with Models

Transformations integrate seamlessly with the dynamics models:

```python
from drone_models.first_principles import dynamics
from drone_models.core import parametrize

# Create parametrized model
model = parametrize(dynamics, "cf2x_L250")

# Use transformations to prepare inputs
thrust_commands = np.array([0.5, 0.6, 0.4, 0.55])  # [N]
rotor_vels = motor_force2rotor_vel(thrust_commands, model.keywords['KF'])

# Run dynamics with transformed inputs
derivatives = model(pos, quat, vel, ang_vel, thrust_commands, rotor_vels)
```

---

*Transformations provide the essential glue between different representations in drone systems, enabling seamless integration across the entire software stack.*
