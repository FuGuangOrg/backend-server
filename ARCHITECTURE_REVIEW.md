# FuGuang 项目架构分析 + To B 销售方案

**分析时间**: 2025-02-28  
**分析师**: Code Agent  
**目标**: 全面review现状 + 提出C/S部署优化 + 设计algo层独立销售方案

---

## 1. 现状分析

### 1.1 三层架构概览

```
fuguang-ui.exe (Qt 6.9 GUI) 
    ↓ TCP port 5555 (JSON over TCP)
fuguang-server.exe (多线程后端)
    ↓ git submodule 静态链接
fuguang-algo (C++20 算法库)
```

### 1.2 GUI层 (inspection-gui)

**技术栈**:
- Qt 6.9.2 + Visual Studio 2022 (Windows)
- TCP客户端 (fiber_end_client)
- 默认连接: `127.0.0.1:5555`

**通信协议**:
- JSON消息通过QTcpSocket发送
- 消息格式: `{ "command": "xxx", "param": {...}, "request_id": "uuid" }`
- 支持远程连接（用户可配置server IP）

**核心功能**:
- 相机设备树管理 (camera_view)
- 参数配置面板 (control_pane) 
- 图像显示+检测结果 (fiber_end_pane)
- 可启动/停止本地server进程

### 1.3 Server层 (backend-server)

**技术栈**:
- Qt 6.9.2 + Visual Studio 2022 + vcpkg
- QTcpServer 监听port 5555
- 多线程架构

**线程架构**:
```cpp
fiber_end_server.exe
├── thread_algorithm      // 算法调度
├── thread_misc          // 相机+自动对焦+图像传输  
├── thread_motion_control // 运动控制
└── thread_device_enum   // 设备枚举
```

**图像传输策略**:
- **同机 (127.0.0.1)**: 共享内存 (`trigger_image`, `detect_image`)
- **跨机**: TCP port 5556 + JPEG压缩流

**依赖关系**:
- 静态链接 `third_party/fuguang-algo` (git submodule)
- 读取 `config.xml` 配置文件
- 支持 `--mock-hardware` 模拟模式

### 1.4 Algo层 (inspection-algo)

**技术栈**:
- C++20 + CMake + vcpkg
- 跨平台算法库 (Linux/macOS/Windows)

**核心算法**:
- 形状匹配: LINEMOD + SIMD (AVX2) 加速
- 目标检测: ResNet18 + FAISS向量搜索  
- 缺陷检测: 局部均值阈值
- 分类判定: IEC 61300-3-35 A/B/C/D区域
- 自动对焦: 多指标锐度 (Laplacian, Tenengrad) + OpenMP

**关键依赖**:
```
OpenCV 4.12+ (图像处理)
ONNX Runtime 1.18+ (ResNet18推理) 
FAISS 1.8+ (向量相似度搜索)
spdlog (日志) + nlohmann/json + pugixml
```

**公共接口**:
```cpp
// 主要参数结构
st_fiber_end_algorithm_parameter {
    double m_pixel_physical_size;        // 像素物理尺寸 um
    int m_detect_type;                   // 检测类型
    int m_mean_kernel_size;              // 局部均值窗口
    vector<st_detect_area> m_detect_areas; // A/B/C/D区域
    // ... 自动对焦、形状匹配等参数
}

// 主要API
class fiber_end_algorithm {
    bool initialize(model_path, faiss_index_path, diameter, params);
    st_detect_box get_shape_match_result(input_image);
    bool get_detect_result(image, center, contours, detect_box);
    bool classification_and_judgment(center, contours, result);
}
```

### 1.5 当前部署方式

1. **开发**: Visual Studio 2022本地编译+调试
2. **发布**: GitHub Actions自动化
   - GUI: Inno Setup → `FuguangSetup_vX.Y.Z.exe`
   - Server: 独立可执行文件 + config.xml
   - Algo: 作为静态库嵌入Server
3. **运行**: GUI启动 → 自动启动本地Server → 通过TCP通信

---

## 2. C/S 架构部署分析

### 2.1 当前架构问题

**优点**:
- GUI-Server已经使用TCP通信，天然支持跨机部署
- 图像传输针对同机/跨机做了优化 (共享内存 vs TCP+JPEG)
- Server支持多客户端连接

**不足**:
- **配置复杂性**: 用户需要手动配置server IP
- **部署依赖**: GUI默认尝试启动本地server，跨机时会失败
- **状态同步**: 多GUI客户端状态可能不一致
- **错误处理**: 网络断线、server宕机恢复机制不够健壮

### 2.2 改进方案

#### 2.2.1 智能连接策略
```cpp
// 在fiber_end_client中实现
enum class ConnectionMode {
    LOCAL_FIRST,    // 优先本地，失败后提示远程
    REMOTE_ONLY,    // 仅连接远程
    AUTO_DISCOVER   // 局域网自动发现
};

class smart_connection_manager {
    bool try_local_connection();
    bool try_remote_connection(QString ip);
    vector<QString> discover_servers_in_lan(); // UDP广播发现
    void setup_heartbeat_monitor();
};
```

#### 2.2.2 高效通信协议优化

**同机通信升级**:
```cpp
// 当前: JSON over TCP
// 建议: 命名管道 + 共享内存
class local_ipc_transport {
    NamedPipe command_pipe;     // 轻量命令
    SharedMemory image_buffer;  // 大数据传输
    Event sync_event;           // 同步原语
};
```

**跨机通信升级**:
```cpp
// 当前: JSON over TCP (port 5555) + JPEG stream (port 5556)
// 建议: gRPC + Protocol Buffers
service FuGuangService {
    rpc SendCommand(CommandRequest) returns (CommandResponse);
    rpc StreamImages(stream ImageRequest) returns (stream ImageData);
}
```

#### 2.2.3 部署配置简化

**GUI端配置向导**:
```
[首次启动]
┌─ 部署模式选择 ─┐
│ ○ 本机模式     │
│ ○ 局域网模式   │  
│ ○ 手动配置     │
└───────────────┘

[局域网模式] → 自动扫描 → 显示可用server列表
[手动配置] → IP:Port输入 → 连接测试
```

**Server端服务发现**:
```cpp
class server_announcement {
    void broadcast_presence();  // UDP广播 (port 5554)
    void respond_to_discovery(); 
    ServerInfo {
        QString hostname, version;
        QStringList supported_cameras;
        bool is_busy;
    };
};
```

---

## 3. To B 销售方案设计

### 3.1 软件License系统

#### 3.1.1 License等级设计

```cpp
enum class LicenseLevel {
    DEMO,           // 演示版：仅预设样本
    BASIC,          // 基础版：基本检测功能
    PROFESSIONAL,   // 专业版：全功能+自定义参数
    ENTERPRISE      // 企业版：SDK接口+源码
};

struct LicenseFeatures {
    bool enable_shape_match;
    bool enable_defect_detection; 
    bool enable_classification;
    bool enable_auto_focus;
    bool enable_custom_parameters;
    bool enable_batch_processing;
    int max_daily_inspections;  // -1 = unlimited
    vector<string> allowed_algorithms; // ResNet18, FAISS等
};
```

#### 3.1.2 License验证机制

**在线验证** (推荐企业客户):
```cpp
class online_license_validator {
    bool validate_license(const string& license_key) {
        // HTTPS POST到license服务器
        // JWT token验证 + 硬件指纹校验
        // 记录使用日志 (IP, 时间戳, 功能调用)
    }
    void periodic_heartbeat();  // 每24小时验证一次
};
```

**离线文件验证** (推荐中小企业):
```cpp
class offline_license_validator {
    bool load_license_file(const string& path) {
        // RSA加密的license文件
        // 包含: 硬件指纹、到期时间、功能开关
        // AES加密algorithm核心参数
    }
    bool verify_hardware_fingerprint();
    bool check_expiry();
};
```

#### 3.1.3 License文件格式

```json
{
  "version": "1.0",
  "customer": {
    "company": "ABC Manufacturing", 
    "contact": "zhang@abc.com"
  },
  "license": {
    "type": "PROFESSIONAL",
    "issue_date": "2025-02-28",
    "expiry_date": "2026-02-28", 
    "max_inspections_per_day": 1000
  },
  "hardware_binding": {
    "cpu_id": "BFEBFBFF000906E9",
    "motherboard_uuid": "03000200-0400-0500-0006-000700080009",
    "mac_addresses": ["aa:bb:cc:dd:ee:ff"],
    "validation_hash": "sha256_of_above_fields"
  },
  "features": {
    "shape_match": true,
    "defect_detection": true, 
    "classification": true,
    "custom_parameters": true,
    "batch_processing": false
  },
  "signature": "RSA_SIGNATURE_OF_ENTIRE_CONTENT"
}
```

### 3.2 硬件绑定方案

#### 3.2.1 硬件指纹收集

```cpp
class hardware_fingerprint_collector {
    string collect_cpu_id() {
        // Windows: CPUID指令
        // Linux: /proc/cpuinfo
        return cpu_signature;
    }
    
    string collect_motherboard_uuid() {
        // Windows: WMI Win32_ComputerSystemProduct.UUID  
        // Linux: dmidecode -s system-uuid
    }
    
    vector<string> collect_mac_addresses() {
        // 排除虚拟网卡、VPN适配器
        // 只收集物理以太网/WiFi MAC
    }
    
    string generate_combined_hash() {
        return sha256(cpu_id + motherboard_uuid + primary_mac);
    }
};
```

#### 3.2.2 加密狗方案评估

**USB Dongle优缺点**:
```
✅ 优点:
- 最强硬件绑定 (物理设备)
- 支持license转移 (插到新机器)
- 可存储加密参数/模型

❌ 缺点:  
- 额外硬件成本 (~200-500元)
- 客户接受度低 (占用USB端口)
- 物流、维护复杂性
- 容易丢失/损坏

💡 建议: 
- 高价值企业客户 (>10万) 可选用加密狗
- 中小客户用软件指纹绑定
```

#### 3.2.3 云端License管理

```cpp
class license_management_portal {
    // 供sales team使用的web管理后台
    void generate_license(customer_info, hardware_fingerprint, features);
    void revoke_license(license_id, reason);
    void extend_license(license_id, new_expiry);
    void track_usage_analytics(license_id);
    
    // 客户自助Portal  
    void request_trial_license(company_info);
    void upload_hardware_fingerprint(); 
    void download_license_file();
};
```

### 3.3 Sample/Demo数据方案

#### 3.3.1 Demo模式设计

```cpp
class demo_data_manager {
    bool initialize_demo_mode() {
        // 加载内嵌的sample images
        load_sample_dataset("builtin://demo_images.zip");
        // 禁用实时相机
        disable_camera_access();
        // 使用预定义参数
        load_demo_parameters();
    }
    
    vector<DemoSample> load_sample_dataset(const string& path) {
        // 10-20张典型端面图像
        // 覆盖不同缺陷类型: 划痕、灰尘、气泡
        // 包含预期检测结果 (ground truth)
    }
};
```

#### 3.3.2 配置文件参数化

**当前问题**: GUI直接发送参数给Server，algo层无独立配置

**解决方案**: 设计标准配置文件格式

```xml
<?xml version="1.0" encoding="UTF-8"?>
<fuguang_config version="1.0">
  <camera>
    <pixel_size unit="um">0.5</pixel_size>
    <exposure_time unit="ms">10</exposure_time>
    <gain>1.2</gain>
  </camera>
  
  <algorithm>
    <shape_match>
      <score_threshold>0.5</score_threshold>
      <gradient_threshold>10.0</gradient_threshold>
      <pyramid_levels>1</pyramid_levels>
    </shape_match>
    
    <defect_detection>
      <detect_type>0</detect_type> <!-- 0=anomaly, 1=scratch -->
      <mean_kernel_size>15</mean_kernel_size>
      <min_area unit="um2">1.0</min_area>
      <enable_dark_anomaly>true</enable_dark_anomaly>
      <enable_bright_anomaly>true</enable_bright_anomaly>
    </defect_detection>
    
    <classification>
      <zones>
        <zone name="A" enabled="true">
          <diameter_min unit="um">0.0</diameter_min>
          <diameter_max unit="um">9.0</diameter_max>
          <max_anomalies>
            <small_dust max_count="0" size_range="0.5-2.0"/>
            <medium_dust max_count="0" size_range="2.0-5.0"/>
            <large_dust max_count="0" size_range="5.0+"/>
          </max_anomalies>
        </zone>
        <!-- B/C/D zones... -->
      </zones>
    </classification>
  </algorithm>
  
  <auto_focus>
    <search_range>100</search_range>
    <move_speed>250</move_speed> 
    <move_step>5</move_step>
  </auto_focus>
</fuguang_config>
```

**独立算法调用**:
```cpp
// 新增命令行工具: fuguang-algo-cli
int main(int argc, char** argv) {
    if (argc < 4) {
        cout << "用法: fuguang-algo-cli config.xml input.jpg output.json\n";
        return 1;
    }
    
    // 1. 加载配置文件
    st_fiber_end_algorithm_parameter params;
    if (!load_config_from_xml(argv[1], params)) {
        cerr << "配置文件加载失败\n";
        return 2;
    }
    
    // 2. 初始化算法
    fiber_end_algorithm algo;
    algo.initialize("models/", "indexes/", 200, &params);
    
    // 3. 处理图像
    cv::Mat image = cv::imread(argv[2], cv::IMREAD_GRAYSCALE);
    auto box = algo.get_shape_match_result(image);
    
    cv::Point center;
    vector<vector<cv::Point>> contours;
    algo.get_detect_result(image, center, contours, box);
    
    st_detect_result result;
    bool pass = algo.classification_and_judgment(center, contours, result);
    
    // 4. 输出JSON结果
    save_result_to_json(argv[3], result, pass);
    
    return pass ? 0 : 3;  // 检测通过=0, 不通过=3
}
```

---

## 4. 实施步骤和优先级

### Phase 1: License系统基础架构 (2-3周)

**P0 - 必须完成**:
- [ ] 设计license文件格式和RSA签名机制
- [ ] 实现硬件指纹收集 (CPU ID + 主板UUID + MAC)
- [ ] 创建offline license验证模块
- [ ] 在algo层添加license检查点 (initialize, get_detect_result)

**P1 - 高优先级**:
- [ ] 开发license生成工具 (sales team使用)
- [ ] 实现功能分级控制 (DEMO/BASIC/PRO/ENTERPRISE)
- [ ] 添加试用期限制 (7天/30天)

### Phase 2: 配置文件参数化 (1-2周)

**P0 - 必须完成**:
- [ ] 设计XML配置文件schema
- [ ] 实现config.xml解析器 (pugixml)
- [ ] 重构algo初始化逻辑，支持配置文件输入
- [ ] 开发fuguang-algo-cli命令行工具

**P1 - 高优先级**:
- [ ] 创建配置文件验证器 (参数范围检查)
- [ ] 添加配置文件加密 (商业参数保护)

### Phase 3: Demo数据和样本模式 (1-2周)

**P0 - 必须完成**:
- [ ] 收集20-30张典型样本图像 (含ground truth)
- [ ] 实现demo模式 (内嵌样本数据)
- [ ] 禁用实时相机访问在demo模式

**P1 - 高优先级**:
- [ ] 创建交互式demo界面
- [ ] 添加样本数据的加密保护

### Phase 4: C/S架构优化 (3-4周)

**P1 - 高优先级**:
- [ ] 实现服务器自动发现 (UDP广播)
- [ ] 优化同机通信 (命名管道替代TCP)
- [ ] 添加连接状态监控和断线重连
- [ ] 实现多客户端状态同步

**P2 - 中等优先级**:
- [ ] 评估gRPC迁移的可行性
- [ ] 开发部署配置向导

### Phase 5: 代码混淆和安全加固 (2-3周)

**P1 - 高优先级**:
- [ ] 核心算法代码混淆 (商业工具如VMProtect)
- [ ] 添加反调试和反逆向机制
- [ ] 关键参数运行时解密

**P2 - 中等优先级**:
- [ ] 在线license验证服务器
- [ ] 用量统计和行为分析

---

## 5. 风险评估和建议

### 5.1 技术风险

**🔴 高风险**:
- **算法模型保护**: ONNX模型文件容易被提取复制
  - *建议*: 模型加密 + 运行时解密
- **License破解**: 软件license容易被逆向
  - *建议*: 多层验证 + 服务器端校验

**🟡 中风险**:
- **性能影响**: License验证可能影响实时检测性能
  - *建议*: 验证结果缓存 + 后台定期检查
- **兼容性**: 硬件指纹在虚拟机/容器环境可能不稳定
  - *建议*: 提供"虚拟化环境"特殊license类型

### 5.2 商业风险

**🔴 高风险**:
- **客户体验**: 过于严格的license限制可能影响客户满意度
  - *建议*: 提供宽松的试用期 + 灵活的license转移政策

**🟡 中风险**:
- **维护成本**: License管理系统需要持续运维
  - *建议*: 设计自动化license生成和分发流程

### 5.3 实施建议

1. **分步推进**: 先实施基础license验证，再逐步添加高级功能
2. **向后兼容**: 确保新版本可以运行旧的配置和license
3. **文档完善**: 为sales team和客户提供详细的部署指南
4. **测试覆盖**: 在多种硬件环境下测试license和硬件绑定机制
5. **监控指标**: 建立license使用统计和异常检测机制

---

## 6. 总结

**现状**: FuGuang项目已具备良好的三层架构基础，GUI-Server通过TCP通信天然支持分布式部署。

**关键改进**:
1. **C/S优化**: 添加服务发现、智能连接、高效IPC通信
2. **License系统**: 多级功能控制 + 硬件绑定 + 离线/在线验证
3. **参数化**: XML配置文件 + 命令行工具，支持algo层独立部署  
4. **Demo模式**: 内嵌样本数据，降低客户试用门槛

**预期效果**:
- 支持灵活的同机/局域网部署
- Algo层可独立打包销售，支持多种license模式
- 降低客户部署和试用复杂度
- 为To B销售提供完整的license管控和防盗版保护

**建议优先级**: License基础架构 > 配置文件参数化 > Demo模式 > C/S优化 > 安全加固
