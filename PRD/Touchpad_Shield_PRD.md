## 一、产品设计目的简述

本软件名称为Touchpad Shield，作用是通过调整笔记本电脑触控板的Click sensitivity、Touchpad sensitivity、Tap with a single finger to single-click、Curtains以及Super Curtains的数值，来减少用户在使用笔记本电脑的键盘打字时因手掌、手腕、大拇指误触触控板而造成的鼠标光标漂移以及误击的现象。本软件严格遵循微软Windows系统Precision Touchpad Devices (touchpad-devices)中关于Precision touchpad tuning (touchpad-tuning-guidelines)的相关优化指南信息进行开发，相关调试优化指南信息请参考以下链接：【https://learn.microsoft.com/en-us/windows-hardware/design/component-guidelines/touchpad-tuning-guidelines】。

## 二、建议的技术栈

前端使用WinUI 3，全栈使用C++进行开发，安装包使用NSIS进行打包。若有更好技术栈可选方案，可以向我提出建议。

## 三、具体需求

### 1、前端UI样式采用与“Windows系统设置”完全一致的风格，界面上主要文字的样式（包括但不限于字体、字号）以及UI界面颜色样式，要直接跟随用户在Windows操作系统中的设定，各控件的样式以及尺寸也要在WinUI 3原生状态下尽量与Windows系统设置界面上的控件保持接近，并且，前端界面必须严格遵循最新Windows系统应用的开发要求，必须正确支持Windows的缩放功能。（本电脑当前Windows系统设置界面案例请见截图）

### 2、前端UI页面布局及交互设计具体要求如下：

（1）本程序前台专用名词中英文对应关系。Sensitivity：灵敏度；Touchpad Sensitivity：触板灵敏度，对应【Windows系统设置-蓝牙和其他设备-触控板】中的“触控板灵敏度”，调用【微软Windows PTP Precision touchpad tuning】中的“Touchpad Sensitivity：触控板灵敏度”，表值为“AAPThreshold”；Click Sensitivity：单击灵敏度，对应【Windows系统设置-蓝牙和其他设备-触控板】中的“单击灵敏度”，调用【微软Windows PTP Precision touchpad tuning】中的“Click Sensitivity：单击灵敏度”，表值为“ClickForceSensitivity”；Smart Area：缓冲区域，对应并调用【微软Windows PTP Precision touchpad tuning】中的“Curtains：屏蔽区域”，表值为“CurtainTop、CurtainLeft、CurtainRight、CurtainBottom”；Smart Edge：防误触区域，对应并调用【微软Windows PTP Precision touchpad tuning】中的“Super Curtains：超级屏蔽区域”，表值为“SuperCurtainTop、SuperCurtainLeft、SuperCurtainRight、SuperCurtainBottom”；Tap with a single finger to single-click：将轻拍触控板的行为判定为单击操作，对应【Windows系统设置-蓝牙和其他设备-触控板】中的“使用单个手指点击即可单击”，调用【微软Windows PTP Precision touchpad tuning】中的“Tap with a single finger to single-click：单指点击以单击”，表值为“TapsEnabled”。

（2）页面主要布局：页面主要分成上、中、下三个区域，最上方区域为软件名称和数据刷新按钮，最下方区域为软件开发者信息及软件版本号，中间区域为程序主要大区，分成左右两个主要部分，左侧是Click sensitivity、Touchpad sensitivity、Curtains以及Super Curtains的参数设定区，右边是触控板以及屏蔽区的示意图，以及触控板尺寸设定区。中左区域再分为上下两个区域，中右区域也再分为上下两个区域。

（3）中左上区域为灵敏度设定区域，从上往下是：单击灵敏度的调节、单击灵敏度控制方式的切换、触板灵敏度的调节、将轻拍触控板的行为判定为单击操作的开关，一共4个功能。
（3.1）单击灵敏度，使用滑块控件，表值为ClickForceSensitivity，可选档位受“单击灵敏度控制方式切换”功能的控制，这里给到两种建议的受控实现方式：第一种，修改滑块刻度，即当“单击灵敏度控制方式切换”设置为“与Windows系统设置保持一致”时，滑块分为三个档位，分别取0、50、100，前台滑块档位显示文字与Windows系统设置保持一致（即“0——轻、50——中、100——重”），对应【Windows系统设置-蓝牙和其他设备-触控板】中的“单击灵敏度”下拉框中的“轻”、“中”、“重”，滑块之后显示具体数值，当“单击灵敏度控制方式切换”设置为“自由调节”时，滑块切换为101个档位（即0～100），滑块在0、50、100处仍然显示为“轻、中、重”，其他档位不显示文字，即样式与“单击灵敏度控制方式切换”设置为“与Windows系统设置保持一致”时保持一样，只是档位从3个变成了101个；第二种，不修改滑块刻度而添加指定吸附值，如果滑块控件有“吸附到指定值”的功能的话，可以将滑块的档位一直设定为101个，但是通过开关吸附0、50、100三个值来控制滑块的操作效果。以上两种方案请看是否都可以实现，如果都可以实现，则推荐使用方案二，即吸附到指定刻度的方案。
（3.2）单击灵敏度控制方式切换，使用下拉框单选控件，有“与Windows系统设置保持一致”和“自由调节”两个枚举值，相关功能见上一条需求说明。
（3.3）触板灵敏度，使用下拉框单选控件，表值为AAPThreshold，分为4个档位，分别取0、1、2、3，0为最高灵敏度、1为高灵敏度、2为中灵敏度、3为低灵敏度，对应【Windows系统设置-蓝牙和其他设备-触控板】中的“单击灵敏度”下拉框中的“最高灵敏度”、“高灵敏度”、“中灵敏度”、“低灵敏度”，用户选择哪个选项，则控件中直接显示所选选项信息。
（3.4）将轻拍触控板的行为判定为单击操作，使用开关控件，表值为TapsEnabled。

（4）中左下区域为屏蔽区域以及超级屏蔽区域设定区域，一共有两个开关，每个开关对应4个输入框。
（4.1）从上往下是：缓冲区域的开关，缓冲区域上、下、左、右数值的输入框，防误触区域的开关，防误触区域上、下、左、右数值输入框，打开【Windows系统设置-蓝牙和其他设备-触摸板】的快捷方式
（4.2）8个输入框内输入的数值的单位均为毫米（mm），小数点后限定为2位，不足2位的填0补位，在传输给后台时需要转换到Himetric数值。
（4.3）当缓冲区域的开关设置为“关”时，程序自动将缓冲区域所有4个数值都设置为0并同时限制用户激活这4个输入框，当缓冲区域的开关设置为“开”时，激活缓冲区域的4个输入框，用户可自由修改对应数值。
（4.4）当防误触区域的开关设置为“关”时，程序自动将防误触区域所有4个数值都设置为0并同时限制用户激活这4个输入框，当防误触区域的开关设置为“开”时，激活防误触区域的4个输入框，用户可自由修改对应数值。
（4.5）打开【Windows系统设置-蓝牙和其他设备-触摸板】的快捷方式的链接文字显示为“更多触控板功能设定”，用户点击改文字后直接打开

（5）中右上区域显示“笔记本电脑型号”、从配置文件中匹配到的触控板物理尺寸，以及应用触控板物理尺寸的按钮
（5.1）笔记本电脑型号，即从【HKLM\HARDWARE\DESCRIPTION\System\BIOS】读取到的“SystemManufacturer”、“SystemProductName”、“SystemSKU”和“SystemVersion”，组合到一起时，“SystemProductName”、“SystemSKU”和“SystemVersion”三个字段前面都加一组字符串【 - 】，即“半角空格 半角减号 半角空格”，如果读取到的某个键值内容为空，则这个键值内容包括相关字符串前面添加的【 - 】都一起隐藏。
（5.2）配置文件中匹配到的物理尺寸，即以从【HKLM\HARDWARE\DESCRIPTION\System\BIOS】读取到的“SystemManufacturer”、“SystemProductName”、“SystemSKU”和“SystemVersion”作为联合主键进行匹配，如果APP中触控板的尺寸已经是匹配到的尺寸，则无需再显示。
（5.3）应用触控板物理尺寸的按钮，当“配置文件中匹配到的物理尺寸”出现时，同步出现本按钮，点击本按钮后，将匹配到的触控板物理尺寸录入到APP的相关设定中并直接应用。

（6）中右中区域显示触控板物理尺寸输入框，有横向尺寸（Width）和纵向尺寸（Height）两个输入框，单位为毫米。启动时先查看APP中是否保存了用户设定的触控板物理尺寸数据，如果保存数据为空，则自动将触控板物理尺寸设定为PTP要求的最小尺寸。

（7）中右下区域显示触控板的示意图，根据最终确定的触控板物理尺寸，结合从edid中读到的显示器物理尺寸、桌面分辨率及放大比例，显示出尽量接近触控板实际尺寸的图样。并且根据用户设定的屏蔽区域以及超级屏蔽区域尺寸，叠加显示对应的屏蔽区域以及超级屏蔽区域图示，屏蔽区为半透明黄色，超级屏蔽区为半透明红色，同时显示对应图例。同时，如果上、下、左、右的超级屏蔽区域大于或者等于屏蔽区域对应区域的设定，则要出现提示信息，告知用户哪个区域的防误触区域大于缓冲区域，该区域将完全以防误触区域的逻辑进行控制。

（8）软件启动时必须请求UAC权限以正常写入屏蔽区域以及超级屏蔽区域的数值。软件启动时应检查注册表中是否有缺失的屏蔽区域以及超级屏蔽区域键值，若有则自动补全并将自动补全的数值设定为0。在完成补全流程后，读取对应数据表并反显在对应控件内，并绘制示意图。

（9）软件的最小窗口尺寸限定为100%缩放下的1280x720。

（10）我在Picture文件夹中放置了一个Touchpad Shield LOGO文件和一个ZiMiaoWorkshop LOGO文件，请以Touchpad Shield LOGO作为本软件的图标，ZiMiaoWorkshopLOGO作为NSIS打包的安装程序的相关图片的设计参考，相关图标在进行缩放时要注意图片精度和色深，要确保相关icon和图片都要清晰。

### 3、配置文件及接口设计

（1）笔记本电脑触控板物理尺寸配置文件命名为TouchpadPhysicalSize，表结构设计为：“SystemManufacturer”，“SystemProductName”，“SystemSKU”，“SystemVersion”，“TouchpadWidth”，“TouchpadHeight”。
（2）针对要调用PTP调优指南中涉及到的注册表键值的功能，在设计相关接口时，接口名称尽量可以直接体现相关键值的名称，例如针对调整ClickForceSensitivity的功能，相关接口命名可以为APP_ClickForceSensitivity。

### 4、版本管理及编译要求（写进rules中强制执行）

（1）版本号格式遵循语义化版本控制，格式为【MAJOR.MINOR.PATCH[-prerelease][+BUILD]】，但要求build构建号并不在每次 CI/CD 构建时自动递增，而是在源码发生变动时才自动递增。MAJOR.MINOR.PATCH由人工指定。
（2）项目文件夹下建立debug、beta和release三个文件夹，分别用来放置最终编译产物。debug下放置编译出来的程序，程序开启debug功能，记录操作日志方便后续debug；beta下放置通过NSIS打包的debug版安装包，安装的也是打开debug功能的程序，记录操作日志方便后续debug；release中放置正式发布的、由NSIS打包的安装包，用于向终端用户发放。
（3）每次修改完源码，请自动编译debug版本，beta版本和release版本在接到我对应指令时再进行编译。
（4）如果需求理解阶段有任何疑问都要与我进行确认，不可自行决定解决方案。