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

insert into user(username,password) values(666666,123456);
insert into user(username,password) values(888888,123456);
insert into user(username,password) values(999999,123456);
insert into user(username,password) values(000000,123456);

insert into customer_info(customer_name) values('zhangsan');
insert into customer_info(customer_name) values('lisi');
insert into model_version(model,version) values('ATA04_Aerodynamics','12.2.1');
insert into model_version(model,version) values('ATA04_Aerodynamics','12.2.2');
insert into model_version(model,version) values('ATA08_WeightBalance','10.2.1');
insert into customer_info(customer_name) values('张三');

insert into user(username,password,last_model_used) values(111111,123456,1);
insert into encryption_key(shell_number) values('qwer1234');
insert into encryption_key(shell_number) values('asdf1234');
insert into product_authorization(encryption_key,authorization_code) values('qwer1234','333444555');
insert into product_authorization(encryption_key,authorization_code) values('asdf1234','333444555');

insert into user_multi_role(user_id,role) values(666666,'模型工程师');
insert into user_multi_role(user_id,role) values(888888,'软件工程师');
测试流程：
