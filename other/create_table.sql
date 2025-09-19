/*
 Navicat Premium Data Transfer

 Source Server         : 172.16.16.67_3306
 Source Server Type    : MySQL
 Source Server Version : 80043 (8.0.43)
 Source Host           : 172.16.16.67:3306
 Source Schema         : model_life_manager

 Target Server Type    : MySQL
 Target Server Version : 80043 (8.0.43)
 File Encoding         : 65001

 Date: 03/09/2025 10:40:28
*/

SET NAMES utf8mb4;
SET FOREIGN_KEY_CHECKS = 0;

-- ----------------------------
-- Table structure for concrete_table_log
-- ----------------------------
DROP TABLE IF EXISTS `concrete_table_log`;
CREATE TABLE `concrete_table_log`  (
  `id` bigint NOT NULL AUTO_INCREMENT,
  `operation_type` varchar(20) CHARACTER SET utf8mb4 COLLATE utf8mb4_general_ci NULL DEFAULT NULL,
  `table_name` varchar(64) CHARACTER SET utf8mb4 COLLATE utf8mb4_general_ci NULL DEFAULT NULL,
  `record_id` bigint NULL DEFAULT NULL,
  `content_before` text CHARACTER SET utf8mb4 COLLATE utf8mb4_general_ci NULL,
  `content_after` text CHARACTER SET utf8mb4 COLLATE utf8mb4_general_ci NULL,
  `operation_log_id` bigint NULL DEFAULT NULL,
  PRIMARY KEY (`id`) USING BTREE,
  INDEX `operation_log_id`(`operation_log_id` ASC) USING BTREE,
  CONSTRAINT `concrete_table_log_ibfk_1` FOREIGN KEY (`operation_log_id`) REFERENCES `operation_log` (`id`) ON DELETE CASCADE ON UPDATE CASCADE
) ENGINE = InnoDB CHARACTER SET = utf8mb4 COLLATE = utf8mb4_general_ci ROW_FORMAT = DYNAMIC;

-- ----------------------------
-- Table structure for customer_info
-- ----------------------------
DROP TABLE IF EXISTS `customer_info`;
CREATE TABLE `customer_info`  (
  `id` bigint NOT NULL AUTO_INCREMENT,
  `customer_name` varchar(128) CHARACTER SET utf8mb4 COLLATE utf8mb4_general_ci NOT NULL,
  `device_type` enum('lab','IPT','FTD','FFS') CHARACTER SET utf8mb4 COLLATE utf8mb4_general_ci NULL DEFAULT NULL,
  `computer_remarks` text CHARACTER SET utf8mb4 COLLATE utf8mb4_general_ci NULL,
  `product_authorization_id` bigint NULL DEFAULT NULL,
  `remarks` text CHARACTER SET utf8mb4 COLLATE utf8mb4_general_ci NULL,
  `suffix` varchar(255) CHARACTER SET utf8mb4 COLLATE utf8mb4_general_ci NULL DEFAULT NULL COMMENT '客户后缀映射，多个用英文逗号分隔',
  PRIMARY KEY (`id`) USING BTREE,
  UNIQUE INDEX `customer_name`(`customer_name` ASC) USING BTREE,
  INDEX `product_authorization_id`(`product_authorization_id` ASC) USING BTREE,
  CONSTRAINT `customer_info_ibfk_1` FOREIGN KEY (`product_authorization_id`) REFERENCES `product_authorization` (`id`) ON DELETE RESTRICT ON UPDATE RESTRICT
) ENGINE = InnoDB AUTO_INCREMENT = 8 CHARACTER SET = utf8mb4 COLLATE = utf8mb4_general_ci COMMENT = '客户表' ROW_FORMAT = DYNAMIC;

-- ----------------------------
-- Table structure for delivery_send
-- ----------------------------
DROP TABLE IF EXISTS `delivery_send`;
CREATE TABLE `delivery_send`  (
  `id` bigint NOT NULL AUTO_INCREMENT,
  `work_order_id` bigint NOT NULL,
  `target_customer` varchar(128) CHARACTER SET utf8mb4 COLLATE utf8mb4_general_ci NULL DEFAULT NULL,
  `validated_by_cae` tinyint(1) NULL DEFAULT NULL,
  `sensitive_info` text CHARACTER SET utf8mb4 COLLATE utf8mb4_general_ci NULL,
  `is_encrypted` tinyint(1) NULL DEFAULT NULL,
  `shell_code` varchar(650) CHARACTER SET utf8mb4 COLLATE utf8mb4_general_ci NULL DEFAULT NULL COMMENT '多个以逗号分隔',
  `remarks` text CHARACTER SET utf8mb4 COLLATE utf8mb4_general_ci NULL,
  `target_delivery_time` datetime NULL DEFAULT NULL COMMENT '预计交付时间',
  `auth_id` varchar(255) CHARACTER SET utf8mb4 COLLATE utf8mb4_general_ci NULL DEFAULT NULL COMMENT '授权id',
  PRIMARY KEY (`id`) USING BTREE,
  INDEX `work_order_id`(`work_order_id` ASC) USING BTREE,
  INDEX `target_customer`(`target_customer` ASC) USING BTREE,
  INDEX `shell_code`(`shell_code` ASC) USING BTREE,
  CONSTRAINT `delivery_send_ibfk_1` FOREIGN KEY (`work_order_id`) REFERENCES `work_order` (`id`) ON DELETE RESTRICT ON UPDATE RESTRICT,
  CONSTRAINT `delivery_send_ibfk_2` FOREIGN KEY (`target_customer`) REFERENCES `customer_info` (`customer_name`) ON DELETE RESTRICT ON UPDATE RESTRICT
) ENGINE = InnoDB AUTO_INCREMENT = 25 CHARACTER SET = utf8mb4 COLLATE = utf8mb4_general_ci COMMENT = '交付发送' ROW_FORMAT = DYNAMIC;

-- ----------------------------
-- Table structure for encryption_key
-- ----------------------------
DROP TABLE IF EXISTS `encryption_key`;
CREATE TABLE `encryption_key`  (
  `id` bigint NOT NULL AUTO_INCREMENT,
  `shell_number` varchar(64) CHARACTER SET utf8mb4 COLLATE utf8mb4_general_ci NULL DEFAULT NULL,
  `shell_serial_number` varchar(64) CHARACTER SET utf8mb4 COLLATE utf8mb4_general_ci NULL DEFAULT NULL,
  `remark` text CHARACTER SET utf8mb4 COLLATE utf8mb4_general_ci NULL,
  `status` enum('出库','入库','损坏','丢失') CHARACTER SET utf8mb4 COLLATE utf8mb4_general_ci NULL DEFAULT '入库',
  PRIMARY KEY (`id`) USING BTREE,
  UNIQUE INDEX `shell_number`(`shell_number` ASC) USING BTREE
) ENGINE = InnoDB AUTO_INCREMENT = 21 CHARACTER SET = utf8mb4 COLLATE = utf8mb4_general_ci COMMENT = '外壳号表' ROW_FORMAT = DYNAMIC;

-- ----------------------------
-- Table structure for encryption_key_history
-- ----------------------------
DROP TABLE IF EXISTS `encryption_key_history`;
CREATE TABLE `encryption_key_history`  (
  `id` bigint NOT NULL AUTO_INCREMENT,
  `encryption_key` varchar(64) CHARACTER SET utf8mb4 COLLATE utf8mb4_general_ci NOT NULL,
  `in_storage_time` datetime NULL DEFAULT NULL,
  `out_storage_time` datetime NULL DEFAULT NULL,
  `status` enum('出库','入库','损坏','丢失') CHARACTER SET utf8mb4 COLLATE utf8mb4_general_ci NOT NULL,
  `customer` varchar(64) CHARACTER SET utf8mb4 COLLATE utf8mb4_general_ci NULL DEFAULT NULL,
  `customer_device_type` enum('lab','IPT','FTD','FFS') CHARACTER SET utf8mb4 COLLATE utf8mb4_general_ci NULL DEFAULT NULL,
  `customer_pc_remark` text CHARACTER SET utf8mb4 COLLATE utf8mb4_general_ci NULL,
  `remark` text CHARACTER SET utf8mb4 COLLATE utf8mb4_general_ci NULL,
  `created_at` datetime NOT NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (`id`) USING BTREE,
  INDEX `encryption_key`(`encryption_key` ASC) USING BTREE,
  INDEX `customer`(`customer` ASC) USING BTREE,
  CONSTRAINT `encryption_key_history_ibfk_1` FOREIGN KEY (`encryption_key`) REFERENCES `encryption_key` (`shell_number`) ON DELETE CASCADE ON UPDATE CASCADE,
  CONSTRAINT `encryption_key_history_ibfk_2` FOREIGN KEY (`customer`) REFERENCES `customer_info` (`customer_name`) ON DELETE CASCADE ON UPDATE CASCADE
) ENGINE = InnoDB AUTO_INCREMENT = 47 CHARACTER SET = utf8mb4 COLLATE = utf8mb4_general_ci COMMENT = '外壳号历史表' ROW_FORMAT = DYNAMIC;

-- ----------------------------
-- Table structure for function_development
-- ----------------------------
DROP TABLE IF EXISTS `function_development`;
CREATE TABLE `function_development`  (
  `id` bigint NOT NULL AUTO_INCREMENT,
  `work_order_id` bigint NOT NULL,
  `description_create` text CHARACTER SET utf8mb4 COLLATE utf8mb4_general_ci NULL,
  `description_completed` text CHARACTER SET utf8mb4 COLLATE utf8mb4_general_ci NULL,
  `model_id` bigint NULL DEFAULT NULL,
  `new_model_version_id` bigint NULL DEFAULT NULL,
  `matlab_version` varchar(255) CHARACTER SET utf8mb4 COLLATE utf8mb4_general_ci NULL DEFAULT NULL COMMENT '基准版本前三位',
  PRIMARY KEY (`id`) USING BTREE,
  INDEX `work_order_id`(`work_order_id` ASC) USING BTREE,
  INDEX `model_id`(`model_id` ASC) USING BTREE,
  INDEX `function_development___fk_4`(`new_model_version_id` ASC) USING BTREE,
  CONSTRAINT `function_development___fk_4` FOREIGN KEY (`new_model_version_id`) REFERENCES `model_version` (`id`) ON DELETE RESTRICT ON UPDATE RESTRICT,
  CONSTRAINT `function_development_ibfk_1` FOREIGN KEY (`work_order_id`) REFERENCES `work_order` (`id`) ON DELETE RESTRICT ON UPDATE RESTRICT,
  CONSTRAINT `function_development_ibfk_2` FOREIGN KEY (`model_id`) REFERENCES `model` (`id`) ON DELETE RESTRICT ON UPDATE RESTRICT
) ENGINE = InnoDB AUTO_INCREMENT = 15 CHARACTER SET = utf8mb4 COLLATE = utf8mb4_general_ci COMMENT = '功能开发工单表' ROW_FORMAT = DYNAMIC;

-- ----------------------------
-- Table structure for history_updatenotes
-- ----------------------------
DROP TABLE IF EXISTS `history_updatenotes`;
CREATE TABLE `history_updatenotes`  (
  `id` bigint NOT NULL AUTO_INCREMENT,
  `model_name` varchar(64) CHARACTER SET utf8mb4 COLLATE utf8mb4_0900_ai_ci NULL DEFAULT NULL,
  `model_version` varchar(64) CHARACTER SET utf8mb4 COLLATE utf8mb4_0900_ai_ci NULL DEFAULT NULL,
  `update_content` text CHARACTER SET utf8mb4 COLLATE utf8mb4_0900_ai_ci NULL,
  PRIMARY KEY (`id`) USING BTREE
) ENGINE = InnoDB AUTO_INCREMENT = 105 CHARACTER SET = utf8mb4 COLLATE = utf8mb4_0900_ai_ci ROW_FORMAT = DYNAMIC;

-- ----------------------------
-- Table structure for issue_reproduction
-- ----------------------------
DROP TABLE IF EXISTS `issue_reproduction`;
CREATE TABLE `issue_reproduction`  (
  `id` bigint NOT NULL AUTO_INCREMENT,
  `work_order_id` bigint NOT NULL,
  `coordination_id` varchar(64) CHARACTER SET utf8mb4 COLLATE utf8mb4_general_ci NULL DEFAULT NULL,
  `description` text CHARACTER SET utf8mb4 COLLATE utf8mb4_general_ci NULL,
  `reference_file` text CHARACTER SET utf8mb4 COLLATE utf8mb4_general_ci NULL,
  `phenomenon` text CHARACTER SET utf8mb4 COLLATE utf8mb4_general_ci NULL,
  `remarks` text CHARACTER SET utf8mb4 COLLATE utf8mb4_general_ci NULL,
  PRIMARY KEY (`id`) USING BTREE,
  INDEX `work_order_id`(`work_order_id` ASC) USING BTREE,
  CONSTRAINT `issue_reproduction_ibfk_1` FOREIGN KEY (`work_order_id`) REFERENCES `work_order` (`id`) ON DELETE RESTRICT ON UPDATE RESTRICT
) ENGINE = InnoDB AUTO_INCREMENT = 46 CHARACTER SET = utf8mb4 COLLATE = utf8mb4_general_ci COMMENT = '问题复现工单表' ROW_FORMAT = DYNAMIC;

-- ----------------------------
-- Table structure for issue_reproduction_attachment
-- ----------------------------
DROP TABLE IF EXISTS `issue_reproduction_attachment`;
CREATE TABLE `issue_reproduction_attachment`  (
  `id` int NOT NULL AUTO_INCREMENT COMMENT '附件ID',
  `ticket_id` bigint NOT NULL COMMENT '关联的工单ID',
  `file_path` varchar(255) CHARACTER SET utf8mb4 COLLATE utf8mb4_general_ci NOT NULL COMMENT '文件保存路径（相对路径）',
  `file_name` varchar(255) CHARACTER SET utf8mb4 COLLATE utf8mb4_general_ci NOT NULL COMMENT '文件原始名称',
  `upload_time` datetime NULL DEFAULT CURRENT_TIMESTAMP COMMENT '附件上传时间',
  PRIMARY KEY (`id`) USING BTREE,
  INDEX `ticket_id`(`ticket_id` ASC) USING BTREE,
  CONSTRAINT `issue_reproduction_attachment_ibfk_1` FOREIGN KEY (`ticket_id`) REFERENCES `work_order` (`id`) ON DELETE RESTRICT ON UPDATE RESTRICT
) ENGINE = InnoDB AUTO_INCREMENT = 10 CHARACTER SET = utf8mb4 COLLATE = utf8mb4_general_ci COMMENT = '附件文件表' ROW_FORMAT = DYNAMIC;

-- ----------------------------
-- Table structure for model
-- ----------------------------
DROP TABLE IF EXISTS `model`;
CREATE TABLE `model`  (
  `id` bigint NOT NULL AUTO_INCREMENT,
  `ata_code` varchar(64) CHARACTER SET utf8mb4 COLLATE utf8mb4_general_ci NULL DEFAULT NULL,
  `model_name` varchar(255) CHARACTER SET utf8mb4 COLLATE utf8mb4_general_ci NULL DEFAULT NULL,
  PRIMARY KEY (`id`) USING BTREE,
  UNIQUE INDEX `ata_code`(`ata_code` ASC) USING BTREE
) ENGINE = InnoDB AUTO_INCREMENT = 25 CHARACTER SET = utf8mb4 COLLATE = utf8mb4_general_ci COMMENT = '模型表' ROW_FORMAT = DYNAMIC;

-- ----------------------------
-- Table structure for model_encryption_authorization
-- ----------------------------
DROP TABLE IF EXISTS `model_encryption_authorization`;
CREATE TABLE `model_encryption_authorization`  (
  `id` bigint NOT NULL AUTO_INCREMENT,
  `model_version_id` bigint NOT NULL,
  `product_authorization_id` bigint NOT NULL,
  PRIMARY KEY (`id`) USING BTREE,
  INDEX `model_version_id`(`model_version_id` ASC) USING BTREE,
  INDEX `product_authorization_id`(`product_authorization_id` ASC) USING BTREE,
  CONSTRAINT `model_encryption_authorization_ibfk_1` FOREIGN KEY (`model_version_id`) REFERENCES `model_version` (`id`) ON DELETE RESTRICT ON UPDATE RESTRICT,
  CONSTRAINT `model_encryption_authorization_ibfk_2` FOREIGN KEY (`product_authorization_id`) REFERENCES `product_authorization` (`id`) ON DELETE RESTRICT ON UPDATE RESTRICT
) ENGINE = InnoDB AUTO_INCREMENT = 1 CHARACTER SET = utf8mb4 COLLATE = utf8mb4_general_ci COMMENT = '模型外壳授权表' ROW_FORMAT = DYNAMIC;

-- ----------------------------
-- Table structure for model_version
-- ----------------------------
DROP TABLE IF EXISTS `model_version`;
CREATE TABLE `model_version`  (
  `id` bigint NOT NULL AUTO_INCREMENT,
  `model` varchar(64) CHARACTER SET utf8mb4 COLLATE utf8mb4_general_ci NOT NULL,
  `version` varchar(64) CHARACTER SET utf8mb4 COLLATE utf8mb4_general_ci NOT NULL,
  `update_time` datetime NOT NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (`id`) USING BTREE,
  UNIQUE INDEX `version`(`version` ASC, `model` ASC) USING BTREE,
  INDEX `model`(`model` ASC) USING BTREE,
  CONSTRAINT `model_version_ibfk_1` FOREIGN KEY (`model`) REFERENCES `model` (`ata_code`) ON DELETE RESTRICT ON UPDATE RESTRICT
) ENGINE = InnoDB AUTO_INCREMENT = 388 CHARACTER SET = utf8mb4 COLLATE = utf8mb4_general_ci COMMENT = '模型版本表' ROW_FORMAT = DYNAMIC;

-- ----------------------------
-- Table structure for operation_log
-- ----------------------------
DROP TABLE IF EXISTS `operation_log`;
CREATE TABLE `operation_log`  (
  `id` bigint NOT NULL AUTO_INCREMENT,
  `user_id` bigint NULL DEFAULT NULL,
  `operation_type` varchar(20) CHARACTER SET utf8mb4 COLLATE utf8mb4_general_ci NULL DEFAULT NULL,
  `table_name` varchar(64) CHARACTER SET utf8mb4 COLLATE utf8mb4_general_ci NULL DEFAULT NULL,
  `record_id` bigint NULL DEFAULT NULL,
  `content_before` text CHARACTER SET utf8mb4 COLLATE utf8mb4_general_ci NULL,
  `content_after` text CHARACTER SET utf8mb4 COLLATE utf8mb4_general_ci NULL,
  `created_at` datetime NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (`id`) USING BTREE,
  INDEX `user_id`(`user_id` ASC) USING BTREE,
  CONSTRAINT `operation_log_ibfk_1` FOREIGN KEY (`user_id`) REFERENCES `user` (`username`) ON DELETE RESTRICT ON UPDATE RESTRICT
) ENGINE = InnoDB AUTO_INCREMENT = 1 CHARACTER SET = utf8mb4 COLLATE = utf8mb4_general_ci ROW_FORMAT = DYNAMIC;

-- ----------------------------
-- Table structure for other_work_order
-- ----------------------------
DROP TABLE IF EXISTS `other_work_order`;
CREATE TABLE `other_work_order`  (
  `id` bigint NOT NULL AUTO_INCREMENT,
  `work_order_id` bigint NOT NULL,
  `description` text CHARACTER SET utf8mb4 COLLATE utf8mb4_general_ci NULL,
  `remarks` text CHARACTER SET utf8mb4 COLLATE utf8mb4_general_ci NULL,
  PRIMARY KEY (`id`) USING BTREE,
  INDEX `work_order_id`(`work_order_id` ASC) USING BTREE,
  CONSTRAINT `other_work_order_ibfk_1` FOREIGN KEY (`work_order_id`) REFERENCES `work_order` (`id`) ON DELETE RESTRICT ON UPDATE RESTRICT
) ENGINE = InnoDB AUTO_INCREMENT = 6 CHARACTER SET = utf8mb4 COLLATE = utf8mb4_general_ci COMMENT = '其他工单表' ROW_FORMAT = DYNAMIC;

-- ----------------------------
-- Table structure for package_send
-- ----------------------------
DROP TABLE IF EXISTS `package_send`;
CREATE TABLE `package_send`  (
  `id` bigint NOT NULL AUTO_INCREMENT,
  `work_order_id` bigint NOT NULL,
  `coordination_id` varchar(64) CHARACTER SET utf8mb4 COLLATE utf8mb4_general_ci NULL DEFAULT NULL,
  `update_content` text CHARACTER SET utf8mb4 COLLATE utf8mb4_general_ci NULL,
  `packaging_requirements` text CHARACTER SET utf8mb4 COLLATE utf8mb4_general_ci NULL,
  `interface_changed` tinyint(1) NULL DEFAULT NULL,
  `target_customer` varchar(128) CHARACTER SET utf8mb4 COLLATE utf8mb4_general_ci NULL DEFAULT NULL,
  `validated_by_cae` tinyint(1) NULL DEFAULT NULL,
  `sensitive_info` text CHARACTER SET utf8mb4 COLLATE utf8mb4_general_ci NULL,
  `new_model_version_id` bigint NULL DEFAULT NULL,
  `is_encrypted` tinyint(1) NULL DEFAULT 0,
  `encryption_key` varchar(650) CHARACTER SET utf8mb4 COLLATE utf8mb4_general_ci NULL DEFAULT NULL COMMENT '多个以英文逗号分割',
  `remarks` text CHARACTER SET utf8mb4 COLLATE utf8mb4_general_ci NULL,
  `target_delivery_time` datetime NULL DEFAULT NULL COMMENT '预计交付时间',
  `matlab_version` varchar(255) CHARACTER SET utf8mb4 COLLATE utf8mb4_general_ci NULL DEFAULT NULL,
  `auth_id` varchar(255) CHARACTER SET utf8mb4 COLLATE utf8mb4_general_ci NULL DEFAULT NULL COMMENT '授权ID',
  PRIMARY KEY (`id`) USING BTREE,
  INDEX `new_model_version_id`(`new_model_version_id` ASC) USING BTREE,
  INDEX `encryption_key`(`encryption_key` ASC) USING BTREE,
  INDEX `target_customer`(`target_customer` ASC) USING BTREE,
  INDEX `work_order_id`(`work_order_id` ASC) USING BTREE,
  CONSTRAINT `package_send_ibfk_1` FOREIGN KEY (`new_model_version_id`) REFERENCES `model_version` (`id`) ON DELETE RESTRICT ON UPDATE RESTRICT,
  CONSTRAINT `package_send_ibfk_4` FOREIGN KEY (`target_customer`) REFERENCES `customer_info` (`customer_name`) ON DELETE RESTRICT ON UPDATE RESTRICT,
  CONSTRAINT `package_send_ibfk_5` FOREIGN KEY (`work_order_id`) REFERENCES `work_order` (`id`) ON DELETE RESTRICT ON UPDATE RESTRICT
) ENGINE = InnoDB AUTO_INCREMENT = 55 CHARACTER SET = utf8mb4 COLLATE = utf8mb4_general_ci COMMENT = '版本迭代+交付发送表' ROW_FORMAT = DYNAMIC;

-- ----------------------------
-- Table structure for product_authorization
-- ----------------------------
DROP TABLE IF EXISTS `product_authorization`;
CREATE TABLE `product_authorization`  (
  `id` bigint NOT NULL AUTO_INCREMENT,
  `encryption_key` varchar(64) CHARACTER SET utf8mb4 COLLATE utf8mb4_general_ci NULL DEFAULT NULL,
  `authorization_code` varchar(64) CHARACTER SET utf8mb4 COLLATE utf8mb4_general_ci NULL DEFAULT NULL,
  `encryption_type` enum('本地锁','网络锁','软锁授权') CHARACTER SET utf8mb4 COLLATE utf8mb4_general_ci NULL DEFAULT NULL,
  `client` varchar(255) CHARACTER SET utf8mb4 COLLATE utf8mb4_general_ci NOT NULL,
  `start_date` date NULL DEFAULT NULL,
  `end_date` date NULL DEFAULT NULL,
  `authorization_status` enum('未授权','授权中','已过期') CHARACTER SET utf8mb4 COLLATE utf8mb4_general_ci NULL DEFAULT '未授权',
  `return` char(1) CHARACTER SET utf8mb4 COLLATE utf8mb4_general_ci NOT NULL DEFAULT '0' COMMENT '0未归还，1已归还',
  `remarks` text CHARACTER SET utf8mb4 COLLATE utf8mb4_general_ci NULL,
  `generate_time` datetime NOT NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (`id`) USING BTREE,
  INDEX `encryption_key`(`encryption_key` ASC) USING BTREE,
  INDEX `client`(`client` ASC) USING BTREE,
  CONSTRAINT `product_authorization_ibfk_1` FOREIGN KEY (`encryption_key`) REFERENCES `encryption_key` (`shell_number`) ON DELETE RESTRICT ON UPDATE RESTRICT,
  CONSTRAINT `product_authorization_ibfk_2` FOREIGN KEY (`client`) REFERENCES `customer_info` (`customer_name`) ON DELETE RESTRICT ON UPDATE RESTRICT
) ENGINE = InnoDB AUTO_INCREMENT = 33 CHARACTER SET = utf8mb4 COLLATE = utf8mb4_general_ci COMMENT = '授权表' ROW_FORMAT = DYNAMIC;

-- ----------------------------
-- Table structure for product_authorization_info
-- ----------------------------
DROP TABLE IF EXISTS `product_authorization_info`;
CREATE TABLE `product_authorization_info`  (
  `id` bigint NOT NULL AUTO_INCREMENT,
  `authorization_id` bigint NOT NULL,
  `encryption_type` enum('本地锁','网络锁','软锁授权') CHARACTER SET utf8mb4 COLLATE utf8mb4_general_ci NOT NULL,
  `authorization_start_date` date NOT NULL,
  `authorization_end_date` date NOT NULL,
  `remark` text CHARACTER SET utf8mb4 COLLATE utf8mb4_general_ci NULL,
  `created_at` datetime NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (`id`) USING BTREE,
  INDEX `authorization_id`(`authorization_id` ASC) USING BTREE,
  CONSTRAINT `product_authorization_info_ibfk_1` FOREIGN KEY (`authorization_id`) REFERENCES `product_authorization` (`id`) ON DELETE CASCADE ON UPDATE CASCADE
) ENGINE = InnoDB AUTO_INCREMENT = 30 CHARACTER SET = utf8mb4 COLLATE = utf8mb4_general_ci COMMENT = '授权详情表' ROW_FORMAT = DYNAMIC;

-- ----------------------------
-- Table structure for user
-- ----------------------------
DROP TABLE IF EXISTS `user`;
CREATE TABLE `user`  (
  `id` bigint NOT NULL AUTO_INCREMENT,
  `username` bigint NOT NULL,
  `password` varchar(255) CHARACTER SET utf8mb4 COLLATE utf8mb4_general_ci NOT NULL,
  `real_name` varchar(64) CHARACTER SET utf8mb4 COLLATE utf8mb4_general_ci NULL DEFAULT NULL,
  `role` enum('SuperUser','模型工程师','软件工程师','访客') CHARACTER SET utf8mb4 COLLATE utf8mb4_general_ci NULL DEFAULT '访客',
  `is_approver` tinyint(1) NULL DEFAULT 0,
  `model_id` bigint NULL DEFAULT NULL,
  `email` varchar(128) CHARACTER SET utf8mb4 COLLATE utf8mb4_general_ci NULL DEFAULT NULL,
  `phone` varchar(32) CHARACTER SET utf8mb4 COLLATE utf8mb4_general_ci NULL DEFAULT NULL,
  `created_at` datetime NULL DEFAULT CURRENT_TIMESTAMP,
  `last_login` datetime NULL DEFAULT NULL,
  `online_status` tinyint(1) NULL DEFAULT 0,
  `last_model_used` bigint NULL DEFAULT NULL,
  PRIMARY KEY (`id`) USING BTREE,
  UNIQUE INDEX `username`(`username` ASC) USING BTREE,
  INDEX `model_id`(`model_id` ASC) USING BTREE,
  CONSTRAINT `user_ibfk_1` FOREIGN KEY (`model_id`) REFERENCES `model` (`id`) ON DELETE RESTRICT ON UPDATE RESTRICT
) ENGINE = InnoDB AUTO_INCREMENT = 21 CHARACTER SET = utf8mb4 COLLATE = utf8mb4_general_ci COMMENT = '用户表' ROW_FORMAT = DYNAMIC;

-- ----------------------------
-- Table structure for user_model
-- ----------------------------
DROP TABLE IF EXISTS `user_model`;
CREATE TABLE `user_model`  (
  `id` bigint NOT NULL AUTO_INCREMENT,
  `username` bigint NOT NULL,
  `model` varchar(64) CHARACTER SET utf8mb4 COLLATE utf8mb4_general_ci NOT NULL,
  PRIMARY KEY (`id`) USING BTREE,
  INDEX `username`(`username` ASC) USING BTREE,
  INDEX `model`(`model` ASC) USING BTREE,
  CONSTRAINT `user_model_ibfk_1` FOREIGN KEY (`username`) REFERENCES `user` (`username`) ON DELETE RESTRICT ON UPDATE RESTRICT,
  CONSTRAINT `user_model_ibfk_2` FOREIGN KEY (`model`) REFERENCES `model` (`ata_code`) ON DELETE RESTRICT ON UPDATE RESTRICT
) ENGINE = InnoDB AUTO_INCREMENT = 261 CHARACTER SET = utf8mb4 COLLATE = utf8mb4_general_ci COMMENT = '用户模型关联表' ROW_FORMAT = DYNAMIC;

-- ----------------------------
-- Table structure for user_multi_role
-- ----------------------------
DROP TABLE IF EXISTS `user_multi_role`;
CREATE TABLE `user_multi_role`  (
  `id` bigint NOT NULL AUTO_INCREMENT,
  `user_id` bigint NOT NULL,
  `role` enum('SuperUser','模型工程师','软件工程师','访客','null') CHARACTER SET utf8mb4 COLLATE utf8mb4_general_ci NOT NULL DEFAULT 'null' COMMENT '系统角色',
  `flow_role` enum('审批人','分发人','执行人','观察者') CHARACTER SET utf8mb4 COLLATE utf8mb4_general_ci NOT NULL COMMENT '在某条流程中的身份',
  `work_order_id` bigint NULL DEFAULT NULL,
  PRIMARY KEY (`id`) USING BTREE,
  INDEX `user_id`(`user_id` ASC) USING BTREE,
  INDEX `work_order_id`(`work_order_id` ASC) USING BTREE,
  CONSTRAINT `user_multi_role_ibfk_1` FOREIGN KEY (`user_id`) REFERENCES `user` (`username`) ON DELETE RESTRICT ON UPDATE RESTRICT,
  CONSTRAINT `user_multi_role_ibfk_2` FOREIGN KEY (`work_order_id`) REFERENCES `work_order` (`id`) ON DELETE RESTRICT ON UPDATE RESTRICT
) ENGINE = InnoDB AUTO_INCREMENT = 450 CHARACTER SET = utf8mb4 COLLATE = utf8mb4_general_ci COMMENT = '用户菜单角色表' ROW_FORMAT = DYNAMIC;

-- ----------------------------
-- Table structure for user_operation_log
-- ----------------------------
DROP TABLE IF EXISTS `user_operation_log`;
CREATE TABLE `user_operation_log`  (
  `id` bigint NOT NULL AUTO_INCREMENT COMMENT '主键ID',
  `user_id` varchar(32) CHARACTER SET utf8mb4 COLLATE utf8mb4_general_ci NOT NULL COMMENT '操作用户ID',
  `request_params` text CHARACTER SET utf8mb4 COLLATE utf8mb4_general_ci NULL COMMENT '请求参数',
  `response_params` text CHARACTER SET utf8mb4 COLLATE utf8mb4_general_ci NULL COMMENT '返回参数',
  `operation_time` datetime NOT NULL DEFAULT CURRENT_TIMESTAMP COMMENT '记录创建时间',
  `operation_url` varchar(255) CHARACTER SET utf8mb4 COLLATE utf8mb4_general_ci NULL DEFAULT NULL,
  PRIMARY KEY (`id`) USING BTREE
) ENGINE = InnoDB AUTO_INCREMENT = 7479 CHARACTER SET = utf8mb4 COLLATE = utf8mb4_general_ci ROW_FORMAT = DYNAMIC;

-- ----------------------------
-- Table structure for version_iteration
-- ----------------------------
DROP TABLE IF EXISTS `version_iteration`;
CREATE TABLE `version_iteration`  (
  `id` bigint NOT NULL AUTO_INCREMENT,
  `work_order_id` bigint NOT NULL,
  `coordination_id` varchar(64) CHARACTER SET utf8mb4 COLLATE utf8mb4_general_ci NULL DEFAULT NULL,
  `update_content` text CHARACTER SET utf8mb4 COLLATE utf8mb4_general_ci NULL,
  `packaging_requirements` text CHARACTER SET utf8mb4 COLLATE utf8mb4_general_ci NULL,
  `interface_changed` tinyint(1) NULL DEFAULT NULL,
  `new_model_version_id` bigint NULL DEFAULT NULL,
  `remarks` text CHARACTER SET utf8mb4 COLLATE utf8mb4_general_ci NULL,
  `matlab_version` varchar(255) CHARACTER SET utf8mb4 COLLATE utf8mb4_general_ci NULL DEFAULT NULL,
  PRIMARY KEY (`id`) USING BTREE,
  INDEX `work_order_id`(`work_order_id` ASC) USING BTREE,
  INDEX `new_model_version_id`(`new_model_version_id` ASC) USING BTREE,
  CONSTRAINT `version_iteration_ibfk_1` FOREIGN KEY (`work_order_id`) REFERENCES `work_order` (`id`) ON DELETE RESTRICT ON UPDATE RESTRICT,
  CONSTRAINT `version_iteration_ibfk_2` FOREIGN KEY (`new_model_version_id`) REFERENCES `model_version` (`id`) ON DELETE RESTRICT ON UPDATE RESTRICT
) ENGINE = InnoDB AUTO_INCREMENT = 21 CHARACTER SET = utf8mb4 COLLATE = utf8mb4_general_ci COMMENT = '版本迭代表' ROW_FORMAT = DYNAMIC;

-- ----------------------------
-- Table structure for work_order
-- ----------------------------
DROP TABLE IF EXISTS `work_order`;
CREATE TABLE `work_order`  (
  `id` bigint NOT NULL AUTO_INCREMENT,
  `creator_id` bigint NOT NULL,
  `created_at` datetime NOT NULL DEFAULT CURRENT_TIMESTAMP,
  `type` enum('问题复现','版本迭代','直接封装+发送','交付发送','功能开发','其他') CHARACTER SET utf8mb4 COLLATE utf8mb4_general_ci NOT NULL,
  `model` varchar(64) CHARACTER SET utf8mb4 COLLATE utf8mb4_general_ci NULL DEFAULT NULL,
  `model_version_id` bigint NULL DEFAULT NULL,
  `status` enum('待审批','待分发','进行中','已完成','已退回') CHARACTER SET utf8mb4 COLLATE utf8mb4_general_ci NULL DEFAULT '待审批',
  `approver_id` bigint NULL DEFAULT NULL,
  `priority` enum('紧急','一般') CHARACTER SET utf8mb4 COLLATE utf8mb4_general_ci NULL DEFAULT NULL,
  `dispatcher_id` bigint NULL DEFAULT NULL,
  `approved_at` datetime NULL DEFAULT NULL,
  `task_priority` enum('紧急','一般') CHARACTER SET utf8mb4 COLLATE utf8mb4_general_ci NULL DEFAULT NULL,
  `dispatched_at` datetime NULL DEFAULT NULL COMMENT '分发人',
  `completed_at` datetime NULL DEFAULT NULL,
  `reject_reason` text CHARACTER SET utf8mb4 COLLATE utf8mb4_general_ci NULL COMMENT '审批拒绝',
  `dispatcher_reject_reason` text CHARACTER SET utf8mb4 COLLATE utf8mb4_general_ci NULL COMMENT '分发拒绝',
  `status_todo` enum('待完成','待封装','待加密','待发送') CHARACTER SET utf8mb4 COLLATE utf8mb4_0900_ai_ci NULL DEFAULT NULL COMMENT '工单待办状态',
  `to_dispatcher_id` bigint NULL DEFAULT NULL COMMENT '分发到',
  `encrypted_remark` varchar(255) CHARACTER SET utf8mb4 COLLATE utf8mb4_general_ci NULL DEFAULT NULL COMMENT '加密备注',
  `send_remark` varchar(255) CHARACTER SET utf8mb4 COLLATE utf8mb4_general_ci NULL DEFAULT NULL COMMENT '发送备注',
  `package_remark` varchar(255) CHARACTER SET utf8mb4 COLLATE utf8mb4_general_ci NULL DEFAULT NULL COMMENT '封装备注',
  PRIMARY KEY (`id`) USING BTREE,
  INDEX `creator_id`(`creator_id` ASC) USING BTREE,
  INDEX `model`(`model` ASC) USING BTREE,
  INDEX `model_version_id`(`model_version_id` ASC) USING BTREE,
  INDEX `approver_id`(`approver_id` ASC) USING BTREE,
  INDEX `dispatcher_id`(`dispatcher_id` ASC) USING BTREE,
  INDEX `type`(`type` ASC) USING BTREE,
  INDEX `type_2`(`type` ASC, `model_version_id` ASC) USING BTREE,
  CONSTRAINT `work_order_ibfk_1` FOREIGN KEY (`creator_id`) REFERENCES `user` (`username`) ON DELETE RESTRICT ON UPDATE RESTRICT,
  CONSTRAINT `work_order_ibfk_2` FOREIGN KEY (`model`) REFERENCES `model` (`ata_code`) ON DELETE RESTRICT ON UPDATE RESTRICT,
  CONSTRAINT `work_order_ibfk_3` FOREIGN KEY (`model_version_id`) REFERENCES `model_version` (`id`) ON DELETE RESTRICT ON UPDATE RESTRICT,
  CONSTRAINT `work_order_ibfk_4` FOREIGN KEY (`approver_id`) REFERENCES `user` (`username`) ON DELETE RESTRICT ON UPDATE RESTRICT,
  CONSTRAINT `work_order_ibfk_5` FOREIGN KEY (`dispatcher_id`) REFERENCES `user` (`username`) ON DELETE RESTRICT ON UPDATE RESTRICT
) ENGINE = InnoDB AUTO_INCREMENT = 41 CHARACTER SET = utf8mb4 COLLATE = utf8mb4_general_ci COMMENT = '所有工单表' ROW_FORMAT = DYNAMIC;

-- ----------------------------
-- Table structure for work_order_executor
-- ----------------------------
DROP TABLE IF EXISTS `work_order_executor`;
CREATE TABLE `work_order_executor`  (
  `id` bigint NOT NULL AUTO_INCREMENT,
  `work_order_id` bigint NOT NULL,
  `executor_id` bigint NOT NULL,
  `create_at` datetime NULL DEFAULT NULL COMMENT '创建时间',
  `transfer_reason` text CHARACTER SET utf8mb4 COLLATE utf8mb4_general_ci NULL,
  `status` enum('封装','加密','流转','发送','完成') CHARACTER SET utf8mb3 COLLATE utf8mb3_general_ci NULL DEFAULT NULL,
  `update_version` varchar(255) CHARACTER SET utf8mb4 COLLATE utf8mb4_general_ci NULL DEFAULT NULL COMMENT '升级后版本',
  `update_remark` varchar(255) CHARACTER SET utf8mb4 COLLATE utf8mb4_general_ci NULL DEFAULT NULL COMMENT '封装备注',
  `product_authorization_id` bigint NULL DEFAULT NULL COMMENT '加密id',
  `product_authorization_remark` varchar(255) CHARACTER SET utf8mb4 COLLATE utf8mb4_general_ci NULL DEFAULT NULL COMMENT '加密备注',
  `create_id` bigint NULL DEFAULT NULL COMMENT '创建id',
  `encryption_status` char(1) CHARACTER SET utf8mb4 COLLATE utf8mb4_general_ci NULL DEFAULT NULL COMMENT '是否发送0未发送1已发送',
  `finish_at` datetime NULL DEFAULT NULL COMMENT '完成时间',
  `transfer_type` enum('封装流转','加密流转','发送流转','完成工单流转') CHARACTER SET utf8mb4 COLLATE utf8mb4_general_ci NULL DEFAULT NULL,
  PRIMARY KEY (`id`) USING BTREE,
  INDEX `work_order_id`(`work_order_id` ASC) USING BTREE,
  INDEX `executor_id`(`executor_id` ASC) USING BTREE,
  CONSTRAINT `work_order_executor_ibfk_1` FOREIGN KEY (`work_order_id`) REFERENCES `work_order` (`id`) ON DELETE RESTRICT ON UPDATE RESTRICT,
  CONSTRAINT `work_order_executor_ibfk_2` FOREIGN KEY (`executor_id`) REFERENCES `user` (`username`) ON DELETE RESTRICT ON UPDATE RESTRICT
) ENGINE = InnoDB AUTO_INCREMENT = 340 CHARACTER SET = utf8mb4 COLLATE = utf8mb4_general_ci COMMENT = '工单执行记录表' ROW_FORMAT = DYNAMIC;

-- ----------------------------
-- Procedure structure for sp_create_delivery_process
-- ----------------------------
DROP PROCEDURE IF EXISTS `sp_create_delivery_process`;
delimiter ;;
CREATE PROCEDURE `sp_create_delivery_process`()
BEGIN
    DECLARE v_model_version_id bigint;
    DECLARE v_work_order_id bigint;
    
    -- 开启事务
    START TRANSACTION;
    
    -- 步骤1: 插入model_version记录
    INSERT INTO model_version (model, version, update_time)
    VALUES ('ATA04_Aerodynamics', '2.0.3.5C', '2024-08-09 00:00:00');
    
    -- 获取刚插入的model_version_id
    SET v_model_version_id = LAST_INSERT_ID();
    
    -- 步骤2: 创建work order并关联model_version_id
    INSERT INTO work_order (creator_id,model_version_id)
		VALUES (666666,v_model_version_id);
    
    -- 获取刚插入的work_order_id
    SET v_work_order_id = LAST_INSERT_ID();
    
    -- 步骤3: 创建delivery send并关联work_order_id及其他字段
    INSERT INTO delivery_send (
        work_order_id, 
        target_customer, 
        shell_code, 
        authorization_id
    ) VALUES (
        v_work_order_id, 
        'CAE',  -- 对应customer_info.CAE
        'A53000000001', 
        '2024071501'
    );
    
    -- 提交事务
    COMMIT;
    
    -- 输出结果
    SELECT '操作成功' AS result, 
           v_model_version_id AS model_version_id, 
           v_work_order_id AS work_order_id;
END
;;
delimiter ;

SET FOREIGN_KEY_CHECKS = 1;
