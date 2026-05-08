# Crazyflie 在环仿真 + Crazyswarm2 编队实验

本仓库包含 Crazyflie SITL 仿真、Crazyswarm2 控制后端、以及编队重配置（formation_reconfiguration）的完整实验链路。已适配到 Ubuntu 24.04 + ROS 2 Jazzy + Gazebo Sim。

## 1. 环境要求

| 组件 | 版本 |
|------|------|
| 操作系统 | Ubuntu 24.04 |
| ROS 2 | Jazzy |
| Gazebo | Gazebo Sim（Jazzy 对应版本） |
| Python | 3.12 |

## 2. 推荐目录结构

```bash
mkdir -p ~/work/crazyswarm2_ws/src
mkdir -p ~/work/crazysim
```

将源码放入对应位置：
- `crazyflie-firmware/` → `~/work/crazysim/crazyflie-firmware`
- `crazyswarm2_ws/src/crazyswarm2/` → `~/work/crazyswarm2_ws/src/crazyswarm2`
- `crazyswarm2_ws/src/formation_reconfiguration/` → `~/work/crazyswarm2_ws/src/formation_reconfiguration`

## 3. 安装配置

### 3.1 Python 虚拟环境

```bash
cd ~/work/crazysim
python3 -m venv .venv
source .venv/bin/activate
pip install -U pip
pip install -r requirements.txt
```

确认虚拟环境可导入这些包：`cflib`、`numpy`、`scipy`、`matplotlib`、`osqp`、`PyYAML`、`transforms3d`、`typeguard`、`Jinja2`

### 3.2 ROS 工作区构建

```bash
cd ~/work/crazyswarm2_ws
source /opt/ros/jazzy/setup.bash
colcon build --symlink-install
source install/setup.bash
```

如果编译报缺包（如 `geometry_msgs`、`crazyflie_interfaces` 等），先用 `apt` 安装对应 ROS 依赖再重新构建。

## 4. 标准运行顺序

**启动顺序很重要，必须严格按以下步骤执行：**

### 4.1 启动仿真固件（SITL）

在 `crazyflie-firmware` 目录下：

```bash
source ~/work/crazysim/.venv/bin/activate
bash tools/crazyflie-simulation/simulator_files/gazebo/launch/sitl_multiagent_square.sh -n 4 -m crazyflie
```

### 4.2 启动 Crazyswarm2 后端

在 `crazyswarm2_ws` 目录下：

```bash
source /opt/ros/jazzy/setup.bash
source install/setup.bash
ros2 launch crazyflie launch.py backend:=cflib gui:=false mocap:=false teleop:=false rviz:=false
```

### 4.3 启动编队控制

```bash
source /opt/ros/jazzy/setup.bash
source install/setup.bash
ros2 launch formation_reconfiguration formation_reconfiguration.launch.py
```

## 5. 实验命令

```bash
# 起飞悬浮 1m
ros2 topic pub --once /all/formation_takeoff std_msgs/msg/Empty "{}"

# 降落
ros2 topic pub --once /all/formation_land std_msgs/msg/Empty "{}"

# 向 x 轴移动 1m
ros2 topic pub --once /all/formation_start std_msgs/msg/Int32 "{data: 1}"

# 对向换位
ros2 topic pub --once /all/formation_start std_msgs/msg/Int32 "{data: 2}"

# 查看历史数据曲线
ros2 run formation_reconfiguration formation_plot_history
```

## 6. 常见坑

- **不要写死 Python 虚拟环境路径**：每个人的机器路径不同，用相对/可配置的方式。
- **真实动捕链路需要 `vrpn_client_ros`**，不做动捕时加 `mocap:=false`。
- **`formation_plot_history` 是交互式绘图入口**，无人值守出图需改成无头保存模式。
- **启动顺序敏感**：必须先拉 4 个 SITL 机器人 → 再启 crazyflie launch → 再启编队节点。
- **不使用 C++ 控制后端**，全链路控制在 Python / `cflib` 侧。

## 7. 技术细节

Ubuntu 24.04 适配所做的主要改动记录在 [ubuntu24_04_adaptation_report.md](ubuntu24_04_adaptation_report.md)，包括固件 SITL 线程超时、陀螺仪 bias 校准跳过、CRTP 扫描包提前回包、编队节点速度兜底等。
