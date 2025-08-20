// 类型导入
import type { Route } from '#/global'
import type { RouteRecordRaw } from 'vue-router'

// 自动生成的文件系统路由和布局处理
import generatedRoutes from 'virtual:generated-pages'
import { setupLayouts } from 'virtual:meta-layouts'

import ClientManage from './delivery/tab.client.manage' // 客户管理路由
import DongleManage from './delivery/tab.dongle.manage' // 加密狗管理路由
import ATA00GroundHandling from './model/tab.ata00.groundhandling' // 地面操纵仿真模型路由
import ATA04Aerodynamics from './model/tab.ata04.aerodynamics' // 气动仿真模型路由
import ATA7xEngine from './model/tab.ata7x.engine' // 发动机模型路由
import ATA08WeightBalance from './model/tab.ata08.weightbalance' // 质量特性仿真模型路由
import ATA21AirConditioning from './model/tab.ata21.airconditioning' // 空调系统仿真模型路由
import ATA21PressutizationAndDoors from './model/tab.ata21.pressutizationanddoors' // 增压系统仿真模型路由
import ATA21_30_36_52_ECS from './model/tab.ata21_30_36_52.ecs' // 大环控模型路由
import ATA24Electric from './model/tab.ata24.electric' // 电源系统仿真模型路由
import ATA26Fire from './model/tab.ata26.fire' // 防火系统仿真模型路由
import ATA27ACEact from './model/tab.ata27.aceact' // ACE作动器模型路由
import ATA27FFlapsSlats from './model/tab.ata27f.flapsslats' // 襟缝翼系统仿真模型路由
import ATA27WStallWarningComputer from './model/tab.ata27w.stallwarningcomputer' // 失速保护计算机仿真模型路由
import ATA28Fuel from './model/tab.ata28.fuel' // 燃油系统仿真模型路由
import ATA29Hydraulic from './model/tab.ata29.hydraulic' // 液压系统仿真模型路由
import ATA30IceAndRain from './model/tab.ata30.iceandrain' // 防冰/除雨系统仿真模型路由
import ATA32Brakes from './model/tab.ata32.brakes' // 刹车系统仿真模型路由
import ATA32LandingGear from './model/tab.ata32.landinggear' // 起落架系统仿真模型路由
import ATA32Steering from './model/tab.ata32.steering' // 前轮转弯系统仿真模型路由
import ATA33Lights from './model/tab.ata33.lights' // 照明系统仿真模型路由
import ATA34FlightInstruments from './model/tab.ata34.flightInstruments' // 飞行仪表系统（大气、无线电）仿真模型路由
import ATA34IRS from './model/tab.ata34.irs' // 惯导基准系统仿真模型路由
import ATA35Oxygen from './model/tab.ata35.oxygen' // 氧气系统仿真模型路由
import ATA36Pneumatic from './model/tab.ata36.penumatic' // 气源系统仿真模型路由
import ATA49APU from './model/tab.ata49.apu' // APU系统仿真模型路由
import OrderCreate from './order/tab.order.create' // 工单创建路由
import OrderList from './order/tab.order.list' // 工单列表路由
import OrderManage from './order/tab.order.manage' // 工单管理路由

// 固定路由（默认路由）
const constantRoutes: RouteRecordRaw[] = [
  {
    path: '/login',
    name: 'login',
    component: () => import('@/views/login.vue'),
    meta: {
      title: '登录',
    },
  },
  {
    path: '/:all(.*)*',
    name: 'notFound',
    component: () => import('@/views/[...all].vue'),
    meta: {
      title: '找不到页面',
    },
  },
]

// 系统路由
const systemRoutes: RouteRecordRaw[] = [
  {
    path: '/',
    component: () => import('@/layouts/index.vue'),
    meta: {
      title: () => useSettingsStore().settings.home.title,
      breadcrumb: false,
    },
    children: [
      {
        path: '',
        component: () => import('@/views/index.vue'),
        meta: {
          title: () => useSettingsStore().settings.home.title,
          icon: 'i-ant-design:home-twotone',
          breadcrumb: false,
        },
      },
      {
        path: 'reload',
        name: 'reload',
        component: () => import('@/views/reload.vue'),
        meta: {
          title: '重新加载',
          breadcrumb: false,
        },
      },
    ],
  },
]

// 动态路由（异步路由、导航栏路由）通常根据权限动态加载
const asyncRoutes: Route.recordMainRaw[] = [
  // {
  //   meta: {
  //     title: '演示',
  //     icon: 'i-uim:box',
  //   },
  //   children: [
  //     permissionExample,
  //   ],
  // },
  {
    meta: {
      title: '模型',
      icon: 'i-icon-park-outline:circular-connection',
    },
    children: [
      ATA04Aerodynamics, // 气动仿真模型
      ATA08WeightBalance, // 质量特性仿真模型
      ATA00GroundHandling, // 地面操纵仿真模型
      ATA21AirConditioning, // 空调系统仿真模型
      ATA21PressutizationAndDoors, // 增压系统仿真模型
      ATA24Electric, // 电源系统仿真模型
      ATA26Fire, // 防火系统仿真模型
      ATA27ACEact, // ACE作动器模型
      ATA27FFlapsSlats, // 襟缝翼系统仿真模型
      ATA27WStallWarningComputer, // 失速保护计算机仿真模型
      ATA28Fuel, // 燃油系统仿真模型
      ATA29Hydraulic, // 液压系统仿真模型
      ATA30IceAndRain, // 防冰/除雨系统仿真模型
      ATA32LandingGear, // 起落架系统仿真模型
      ATA32Steering, // 前轮转弯系统仿真模型
      ATA32Brakes, // 刹车系统仿真模型
      ATA33Lights, // 照明系统仿真模型
      ATA34IRS, // 惯导基准系统仿真模型
      ATA34FlightInstruments, // 飞行仪表系统仿真模型
      ATA35Oxygen, // 氧气系统仿真模型
      ATA36Pneumatic, // 气源系统仿真模型
      ATA49APU, // APU系统仿真模型
      ATA21_30_36_52_ECS, // 大环控模型仿真模型
      ATA7xEngine, // 发动机模型仿真模型
    ],
  },
  {
    meta: {
      title: '工单',
      icon: 'i-material-symbols:order-approve-outline',
    },
    children: [
      OrderCreate, // 工单创建路由
      OrderManage, // 工单待办路由
      OrderList, // 工单列表路由
    ],
  },
  {
    meta: {
      title: '交付',
      icon: 'i-material-symbols:key-outline',
    },
    children: [
      DongleManage, // 加密狗管理路由
      ClientManage, // 客户管理路由
    ],
  },
]

// 基于文件系统自动生成的固定路由
const constantRoutesByFilesystem = generatedRoutes.filter((item) => {
  return item.meta?.enabled !== false && item.meta?.constant === true
})

// 基于文件系统自动生成的动态路由，并自动挂载布局
const asyncRoutesByFilesystem = setupLayouts(generatedRoutes.filter((item) => {
  return item.meta?.enabled !== false && item.meta?.constant !== true && item.meta?.layout !== false
}))

// 导出所有路由表，供主路由文件使用
export {
  asyncRoutes, // 手动配置的动态路由
  asyncRoutesByFilesystem, // 文件系统生成的动态路由
  constantRoutes, // 手动配置的固定路由
  constantRoutesByFilesystem, // 文件系统生成的固定路由
  systemRoutes, // 系统基础路由
}
