import type { RecursiveRequired, Settings } from '#/global'
import { cloneDeep } from 'es-toolkit'
import settingsDefault from '@/settings.default'
import { merge } from '@/utils/object'

const globalSettings: Settings.all = {
  app: {
    enablePermission: true, // 是否启用权限控制
    enableDynamicTitle: true, // 是否启用动态页面标题
    // routeBaseOn: 'filesystem', // 路由数据来源（文件系统）
  },
  layout: {
    enableMobileAdaptation: true, // 是否启用移动端适配
  },
  menu: {
    enableSubMenuCollapseButton: true, // 是否显示子菜单折叠按钮
    enableHotkeys: true, // 是否启用菜单快捷键
  },
  topbar: {
    mode: 'fixed', // 顶部栏模式（固定）
  },
  tabbar: {
    enable: true, // 是否启用多标签栏
    enableIcon: true, // 标签栏是否显示图标
    enableHotkeys: true, // 是否启用标签栏快捷键
  },
  toolbar: {
    fullscreen: true, // 是否启用全屏按钮
    pageReload: true, // 是否启用页面刷新按钮
    colorScheme: true, // 是否启用主题色切换
  },
  mainPage: {
    enableHotkeys: true, // 主页面是否启用快捷键
  },
  // copyright: {
  //   enable: true, // 是否显示版权信息
  //   dates: '2020-present', // 版权年份
  //   company: 'Fantastic-admin', // 公司名称
  //   website: 'https://fantastic-admin.hurui.me', // 公司官网
  // },
}

export default merge(globalSettings, cloneDeep(settingsDefault)) as RecursiveRequired<Settings.all>
