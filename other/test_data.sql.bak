use Model_Life_Manager;
insert into model(ata_code, model_name) values
('ATA04_Aerodynamics', '气动仿真模型'),
('ATA08_WeightBalance', '质量特性仿真模型'),
('ATA00_Groundhandling', '地面操纵仿真模型'),
('ATA21_AirConditioning', '空调系统仿真模型'),
('ATA21_PressutizationAndDoors', '增压系统仿真模型'),
('ATA24_Electric', '电源系统仿真模型'),
('ATA26_Fire', '防火系统仿真模型'),
('ATA27_ACEact', 'ACE作动器模型'),
('ATA27F_FlapsSlats', '襟缝翼系统仿真模型'),
('ATA27W_StallWarningComputer', '失速保护计算机仿真模型'),
('ATA28_Fuel', '燃油系统仿真模型'),
('ATA29_Hydraulic', '液压系统仿真模型'),
('ATA30_IceAndRain', '防冰/除雨系统仿真模型'),
('ATA32_LandingGear', '起落架系统仿真模型'),
('ATA32_Steering', '前轮转弯系统仿真模型'),
('ATA32_Brakes', '刹车系统仿真模型'),
('ATA33_Lights', '照明系统仿真模型'),
('ATA34_IRS', '惯导基准系统仿真模型'),
('ATA34_FlightInstruments', '飞行仪表系统（大气、无线电）'),
('ATA35_Oxygen', '氧气系统仿真模型'),
('ATA36_Penumatic', '气源系统仿真模型'),
('ATA49_APU', 'APU系统仿真模型'),
('ATA21/30/36/52_ECS', '大环控模型'),
('ATA7x_Engine', '发动机模型');

-- 一些用户
insert into user(username,password) values(666666,123456);
insert into user(username,password) values(888888,123456);
insert into user(username,password) values(999999,123456);
insert into user(username,password) values(000000,123456);
insert into user(username,password,last_model_used) values(111111,123456,1);


-- 客户信息
insert into customer_info(customer_name) values('zhangsan');
insert into customer_info(customer_name) values('lisi');

-- 模型版本
insert into model_version(model,version) values('ATA04_Aerodynamics','12.2.1');
insert into model_version(model,version) values('ATA04_Aerodynamics','12.2.2');
insert into model_version(model,version) values('ATA08_WeightBalance','10.2.1');


-- 加密狗
insert into encryption_key(shell_number) values('qwer1234');
insert into encryption_key(shell_number) values('asdf1234');

-- 授权ID
insert into product_authorization(encryption_key,authorization_code) values('qwer1234','333444555');
insert into product_authorization(encryption_key,authorization_code) values('asdf1234','333444555');

-- 指定用户身份
insert into user_multi_role(user_id,role) values(666666,'模型工程师');
insert into user_multi_role(user_id,role) values(888888,'软件工程师');

-- 指定一些预先填入的工单流程中的角色(不涉及到具体的工单)
insert into user_multi_role(user_id,flow_role) values(666666,'审批人');
insert into user_multi_role(user_id,flow_role) values(666666,'分发人');
insert into user_multi_role(user_id,flow_role) values(888888,'分发人');
insert into user_multi_role(user_id,flow_role) values(888888,'执行人');


-- 一些工单的测试数据
INSERT INTO work_order (
    creator_id, created_at, type, model, model_version_id,
    status, approver_id, priority, dispatcher_id,
    approved_at, task_priority, dispatched_at, completed_at, reject_reason
) VALUES 
(
    666666, NOW(), '问题复现','ATA04_Aerodynamics', 1,
    '待审批', 666666, '一般', NULL,
    NULL, NULL, NULL, NULL, NULL
);
SET @wid = LAST_INSERT_ID();

INSERT INTO issue_reproduction (
    work_order_id,
    coordination_id,
    description,
    reference_file,
    phenomenon,
    remarks
) VALUES (
    @wid,
    'COORD-20250729-001',
    '软件启动后无法正常加载模型文件',
    'D:/reference_data/sample_model.dat',
    '程序崩溃，日志显示内存访问违规',
    '测试环境为Windows 10，使用MySQL 8.0，问题具备稳定复现性'
);

INSERT INTO work_order_executor (
    work_order_id, executor_id, transferred_at, transfer_reason
) VALUES 
(@wid, 666666, '2025-07-29 10:00:00', '自动分配'),
(@wid, 888888, '2025-07-29 10:10:00', '第一处理人转移'),
(@wid, 000000, '2025-07-29 11:00:00', '专家接手分析');


INSERT INTO issue_reproduction_attachment (
    ticket_id, file_path, file_name
) VALUES (
    @wid,
    '/ticket/aaa.txt',
    'log.txt'
);


INSERT INTO work_order (
    creator_id, created_at, type, model, model_version_id,
    status, approver_id, priority, dispatcher_id,
    approved_at, task_priority, dispatched_at, completed_at, reject_reason
) VALUES 
(
    666666, NOW(), '版本迭代','ATA04_Aerodynamics', 1,
    '待审批', 666666, '一般', NULL,
    NULL, NULL, NULL, NULL, NULL
);
SET @wid = LAST_INSERT_ID();

INSERT INTO version_iteration (
    work_order_id,
    coordination_id,
    update_content,
    packaging_requirements,

    remarks
) VALUES (
    @wid,
    'COORD-20250729-002',
    '修复数据导出精度错误问题，优化渲染性能',
    '需压缩为zip包，放置于release目录，附带更新说明',

    '涉及核心模块接口变更，需通知客户并同步文档'
);


SET @wid = LAST_INSERT_ID();
INSERT INTO work_order (
    creator_id, created_at, type, model, model_version_id,
    status, approver_id, priority, dispatcher_id,
    approved_at, task_priority, dispatched_at, completed_at, reject_reason
) VALUES 
(
    666666, NOW(), '功能开发','ATA04_Aerodynamics', 1,
    '待审批', 666666, '一般', NULL,
    NULL, NULL, NULL, NULL, NULL
);

INSERT INTO function_development (
    work_order_id, description_create, description_completed
) VALUES (
    @wid,
    '新增用户角色管理功能',
    '功能开发完成，已提交测试'
);
INSERT INTO work_order (
    creator_id, created_at, type, model, model_version_id,
    status, approver_id, priority, dispatcher_id,
    approved_at, task_priority, dispatched_at, completed_at, reject_reason
) VALUES 
(
    666666, NOW(), '其他','ATA04_Aerodynamics', 1,
    '待审批', 666666, '一般', NULL,
    NULL, NULL, NULL, NULL, NULL
);
SET @wid = LAST_INSERT_ID();
INSERT INTO other_work_order (
    work_order_id, description, remarks
) VALUES (
    @wid,
    '紧急处理客户反馈问题',
    '该工单不属于常规分类，记录处理过程'
);
SET @wid = LAST_INSERT_ID();
INSERT INTO work_order (
    creator_id, created_at, type, model, model_version_id,
    status, approver_id, priority, dispatcher_id,
    approved_at, task_priority, dispatched_at, completed_at, reject_reason
) VALUES 
(
    666666, NOW(), '交付发送','ATA04_Aerodynamics', 1,
    '待审批', 666666, '一般', NULL,
    NULL, NULL, NULL, NULL, NULL
);
SET @wid = LAST_INSERT_ID();

INSERT INTO delivery_send (
    work_order_id, target_customer, validated_by_cae, sensitive_info, is_encrypted,
    shell_code, authorization_id, remarks
) VALUES (
    @wid,
    '张三',
    TRUE,
    '包含部分未公开接口文档',
    TRUE,
    'qwer1234',
	1,
    '经CAE验证，已交付客户'
);

INSERT INTO work_order (
    creator_id, created_at, type, model, model_version_id,
    status, approver_id, priority, dispatcher_id,
    approved_at, task_priority, dispatched_at, completed_at, reject_reason
) VALUES 
(
    666666, NOW(), '直接封装+发送','ATA04_Aerodynamics', 1,
    '待审批', 666666, '一般', NULL,
    NULL, NULL, NULL, NULL, NULL
);
SET @wid = LAST_INSERT_ID();
INSERT INTO package_send (
    work_order_id, coordination_id, update_content, packaging_requirements, interface_changed,
    target_customer, validated_by_cae, sensitive_info, new_model_version_id,
    is_encrypted, encryption_key, product_authorization_id, remarks
) VALUES (
    @wid,
    'COORD-20250729-003',
    '优化数据传输协议，新增压缩选项',
    '请打包为tar.gz格式，附带部署文档',
    TRUE,
    'zhangsan',
    TRUE,
    '涉及通信加密算法，内容敏感',
    2,
    TRUE,
    'qwer1234',
    1,
    '完成打包并通知客户，等待验收'
);

select * from work_order order by id desc limit 0,INT_MAX; 