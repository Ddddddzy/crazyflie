# CrazySim / Crazyswarm2 24.04 适配报告

> **新手入门请先看 [readme](readme)**，那里有完整的环境配置和运行步骤。本文档记录的是从 22.04 到 24.04 所做的技术改动细节，供深入了解或排查问题时参考。

## 1. 适配目标

本次工作目标是把原本面向 Ubuntu 22.04 + Gazebo Garden 的仿真链路，迁移到 Ubuntu 24.04 环境下，并保持以下流程可用：

- `crazyflie-firmware` 在环仿真可启动
- `crazyswarm2` 通过 `cflib` 后端可连接仿真机
- `formation_reconfiguration` 可执行起飞、编队切换、降落
- 历史数据可保存并生成图像

全流程没有使用 C++ 作为控制后端，控制链路保持在 Python / `cflib` 侧。

## 2. 为 24.04 做的改动

### 2.1 固件 SITL 线程改成有限等待

文件：
- [`crazyflie-firmware/src/hal/src/socketlink.c`](./crazyflie-firmware/src/hal/src/socketlink.c)

改动：
- 将 `socketlinkTask()` 里的 `poll()` 改为有限超时等待，而不是无限阻塞。
- 增加 `EINTR` 处理。

原因：
- Ubuntu 24.04 上 POSIX/FreeRTOS 的任务调度行为更容易让固件卡在无限等待里。
- 如果 `socketlink` 卡死，固件无法继续转发 CRTP / 传感器流量，`cflib` 也就无法稳定完成握手。

### 2.2 SITL 启动时直接放过陀螺仪 bias 校准门槛

文件：
- [`crazyflie-firmware/src/hal/src/sensors_sitl.c`](./crazyflie-firmware/src/hal/src/sensors_sitl.c)

改动：
- 在 `sensorsSimInit()` 中将 `gyroBiasFound` 直接置为 `true`，并把 `gyroBias` 置零。

原因：
- 物理机上的 IMU bias 找零流程不适合直接照搬到 SITL。
- 在 24.04 + 新版仿真栈下，这个门槛会拖住启动，导致 `cflib` 迟迟连不上。

### 2.3 CrazySim 插件支持 `cflib` 扫描包更早返回

文件：
- [`crazyflie-firmware/tools/crazyflie-simulation/simulator_files/gazebo/plugins/CrazySim/crazysim_plugin.cpp`](./crazyflie-firmware/tools/crazyflie-simulation/simulator_files/gazebo/plugins/CrazySim/crazysim_plugin.cpp)

改动：
- 对 `0xFF` 的空 CRTP 扫描包直接回包。
- 这个回包发生在固件握手完成之前。

原因：
- `cflib` 的扫描 / 探测流程需要尽早拿到响应。
- 旧逻辑过于依赖固件握手完成，容易在 24.04 下出现发现不稳定或首次连接失败。

### 2.4 编队节点对“速度话题晚到”做了兜底

文件：
- [`crazyswarm2_ws/src/formation_reconfiguration/formation_reconfiguration/formation_reconfiguration_node.py`](./crazyswarm2_ws/src/formation_reconfiguration/formation_reconfiguration/formation_reconfiguration_node.py)

改动：
- 在 `_update_pose_state()` 中，如果 Pose 先到、速度话题还没来，就先把速度置零并标记为已收到。

原因：
- 24.04 下启动顺序和话题到达时序更敏感。
- 编队节点不应因为“速度话题还没来”而阻塞整个控制链。

### 2.5 历史数据读取与绘图入口整理

文件：
- [`crazyswarm2_ws/src/formation_reconfiguration/formation_reconfiguration/plot_history.py`](./crazyswarm2_ws/src/formation_reconfiguration/formation_reconfiguration/plot_history.py)

现状：
- 这个入口默认读取 `logs/` 里最新的 `.npz` 历史文件。
- 它本身是交互式绘图入口，原始行为会调用 `plt.show()`。

说明：
- 为了自动生成结果图，我实际执行时改用无头绘图方式把图保存成 `png`。
- 如果你后续想把“自动保存图片”也做成正式功能，可以再给这个入口加一个 `--save` 参数。

## 3. 交付给别人时，应该发哪些东西

如果你准备把 `src` 之类的东西发给别人，最小建议是发这几部分：

- `crazyflie-firmware/`
- `crazyswarm2_ws/src/formation_reconfiguration/`
- `crazyswarm2_ws/src/crazyswarm2/`
- `crazyflie-lib-python/`
- `requirements.txt`

如果对方只是做这个实验流程，`formation_ui/`、`motion_capture_tracking/` 这类包不是必须的，除非你要保留 UI 或真实动捕链路。

## 4. 别人本机需要装什么

### 4.1 系统环境

建议对方直接用：

- Ubuntu 24.04
- ROS 2 Jazzy
- Gazebo / Gazebo Sim 对应 Jazzy 版本
- Python 3.12 系列环境

### 4.2 Python 依赖

你当前仓库里这套实验实际依赖的 Python 包，至少包括：

- `cflib`
- `numpy`
- `scipy`
- `matplotlib`
- `osqp`
- `PyYAML`
- `transforms3d`
- `typeguard`
- `Jinja2`

仓库根目录的 `requirements.txt` 也已经列了核心依赖。

### 4.3 ROS 依赖

从 `formation_reconfiguration/package.xml` 看，至少需要：

- `rclpy`
- `ament_index_python`
- `geometry_msgs`
- `crazyflie_interfaces`
- `motion_capture_tracking_interfaces`

如果要走真实动捕链路，还需要对应动捕客户端包；如果只做 Gazebo 在环仿真，可以直接用 `mocap:=false`，不必强依赖动捕服务。

## 5. 别人拿到包之后怎么配置

### 5.1 推荐目录结构

假设别人也有一个工作区，建议这样放：

```bash
mkdir -p ~/work/crazyswarm2_ws/src
mkdir -p ~/work/crazysim
```

把你发过去的源码分别放入：

- `~/work/crazysim/crazyflie-firmware`
- `~/work/crazyswarm2_ws/src/crazyswarm2`
- `~/work/crazyswarm2_ws/src/formation_reconfiguration`
- 需要的话再放 `crazyflie-lib-python`

### 5.2 Python 虚拟环境

不要固定写死你机器上的 `/home/l/venv2`。

更稳妥的方式是让对方自己建一个项目虚拟环境，例如：

```bash
cd ~/work/crazysim
python3 -m venv .venv
source .venv/bin/activate
pip install -U pip
pip install -r requirements.txt
```

然后确保该虚拟环境里能导入：

- `cflib`
- `numpy`
- `scipy`
- `matplotlib`
- `osqp`

如果对方是自己从源码安装 `cflib`，也可以直接把 `crazyflie-lib-python/` 安装进这个环境。

### 5.3 ROS 工作区构建

在 `crazyswarm2_ws` 下：

```bash
source /opt/ros/jazzy/setup.bash
colcon build --symlink-install
source install/setup.bash
```

如果对方第一次编译报缺包，先补 ROS 依赖，再重新 `colcon build`。

## 6. 标准运行顺序

### 6.1 先启动仿真固件

在 `crazyflie-firmware` 目录下：

```bash
source ~/work/crazysim/.venv/bin/activate
bash tools/crazyflie-simulation/simulator_files/gazebo/launch/sitl_multiagent_square.sh -n 4 -m crazyflie
```

### 6.2 再启动 Crazyswarm2 后端

在 `crazyswarm2_ws` 目录下：

```bash
source /opt/ros/jazzy/setup.bash
source install/setup.bash
ros2 launch crazyflie launch.py backend:=cflib gui:=false mocap:=false teleop:=false rviz:=false
```

### 6.3 再启动编队控制

```bash
source /opt/ros/jazzy/setup.bash
source install/setup.bash
ros2 launch formation_reconfiguration formation_reconfiguration.launch.py
```

### 6.4 实验命令

起飞悬浮 1m：

```bash
ros2 topic pub --once /all/formation_takeoff std_msgs/msg/Empty "{}"
```

降落：

```bash
ros2 topic pub --once /all/formation_land std_msgs/msg/Empty "{}"
```

切换编队：

```bash
ros2 topic pub --once /all/formation_start std_msgs/msg/Float32MultiArray "{data: [x, y]}"
```

其中：

- `x` 是基础队形
- `y` 是附加变换

历史图查看：

```bash
ros2 run formation_reconfiguration formation_plot_history
```

## 7. 需要提醒别人的坑

- 不要把 Python 虚拟环境路径写死成你机器上的绝对路径。
- `vrpn_client_ros` 不一定默认存在，真实动捕链路要单独装；不做动捕时直接 `mocap:=false`。
- `formation_plot_history` 是交互式入口，如果要无人值守出图，建议改成无头保存模式。
- 这套实验对“启动顺序”敏感，建议先把 4 个 SITL 机器人拉起来，再启动 `crazyflie` launch，再启动编队节点。

