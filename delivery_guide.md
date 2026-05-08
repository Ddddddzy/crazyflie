# CrazySim 交付说明

## 1. 交付范围

如果要把这套仿真和实机代码发给别人，建议只发源码和配置，不发构建产物。

### 建议发送

- `crazyflie-firmware/`
- `crazyswarm2_ws/src/crazyswarm2/`
- `crazyswarm2_ws/src/formation_reconfiguration/`
- `crazyswarm2_ws/src/formation_ui/`，如果对方也需要 Web 控制页
- `crazyflie-lib-python/`，如果对方需要独立使用 `cflib`
- `requirements.txt`
- `requirements-extra.txt`，如果对方还需要额外实验包
- 你的实验启动说明文档，比如 `readme` 或整理后的 README

### 不建议发送

- `build/`
- `install/`
- `log/`
- `.venv/`
- 任何临时缓存目录

这些内容都和本机环境强绑定，换机器后通常需要重新构建。

## 2. 对方是否需要重新构建

### `crazyflie-firmware`

需要。

这部分不是直接运行 Python，而是要把 SITL 固件源码编译成可执行文件。
标准流程是：

```bash
cd crazyflie-firmware
mkdir -p sitl_make/build
cd sitl_make/build
cmake ..
make -j"$(nproc)" all
```

生成的 `cf2` 是本机二进制，不能直接跨机器复用。

### `crazyswarm2_ws`

需要。

ROS 2 包里的消息接口、Python entrypoint、C++ 节点，都会在构建时生成。
标准流程是：

```bash
cd crazyswarm2_ws
source /opt/ros/jazzy/setup.bash
source /path/to/venv/bin/activate
colcon build --symlink-install
source install/setup.bash
```

## 3. 对方本机需要准备什么

### 系统环境

- Ubuntu 24.04
- ROS 2 Jazzy
- Gazebo 对应版本
- Python 3.12 环境

### Python 依赖

至少需要：

- `cflib`
- `numpy`
- `scipy`
- `matplotlib`
- `osqp`
- `PyYAML`
- `transforms3d`
- `typeguard`
- `Jinja2`
- `empy`
- `catkin_pkg`

### ROS 依赖

至少需要：

- `rclpy`
- `ament_index_python`
- `geometry_msgs`
- `crazyflie_interfaces`
- `motion_capture_tracking_interfaces`

如果要用真实动捕，还需要对应的 VRPN 或 motion capture 客户端包。

## 4. 启动顺序

### 仿真

```bash
cd crazyflie-firmware
source /path/to/venv/bin/activate
bash tools/crazyflie-simulation/simulator_files/gazebo/launch/sitl_multiagent_square.sh -n 1 -m crazyflie
```

```bash
cd crazyswarm2_ws
source /path/to/venv/bin/activate
source /opt/ros/jazzy/setup.bash
source install/setup.bash
ros2 launch crazyflie launch.py \
  backend:=cflib gui:=false mocap:=false teleop:=false rviz:=false \
  crazyflies_yaml_file:=src/crazyswarm2/crazyflie/config/virtual.config
```

### 实机

```bash
cd crazyswarm2_ws
source /path/to/venv/bin/activate
source /opt/ros/jazzy/setup.bash
source install/setup.bash
ros2 launch vrpn_client_ros sample.launch.py
```

```bash
ros2 launch crazyflie launch.py \
  backend:=cflib gui:=false mocap:=true teleop:=false rviz:=false \
  crazyflies_yaml_file:=src/crazyswarm2/crazyflie/config/real.config \
  motion_capture_yaml_file:=src/crazyswarm2/crazyflie/config/motion_capture.yaml
```

## 5. 备注

- 如果对方只跑 `mocap:=false` 的实机流程，`motion_capture_tracking` 本体可以不启动，但接口包和控制包不能乱删。
- `motion_capture_tracking_interfaces` 是生成头文件和 ROS 消息类型的关键依赖，删掉会导致 `vrpn_client_ros` 和相关控制包编译失败。
- 最稳妥的交付方式是只发源码，让对方在自己的机器上按同样步骤重建。
