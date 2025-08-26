import type { RouteRecordRaw } from 'vue-router'

function Layout() {
  return import('@/layouts/index.vue')
}

const routes: RouteRecordRaw = {
  path: '/ata36_pneumatic', // 一级路由路径
  component: Layout,
  name: 'ATA36_Pneumatic', // 路由名称，唯一标识
  meta: {
    title: 'ATA36_气源系统仿真模型', // 菜单或标签页显示的标题
    icon: 'i-ic:round-tab', // 菜单或标签页显示的图标
    auth: 'ATA36_Penumatic.browse', // 需要的具体权限
  },
  children: [
    {
      path: '', // 默认子路由，访问 /ata08_weightbalance 时自动加载
      name: 'ATA36_Pneumatic_Info', // 子路由名称，唯一标识 这里不重要
      component: () => import('@/views/model/model_list.vue'), // 具体页面组件
      meta: {
        title: 'ATA36_气源系统仿真模型', // 页面标题
        menu: false, // 不在菜单中显示
        breadcrumb: false, // 不在面包屑中显示
        activeMenu: '/ata36_pneumatic', // 激活菜单项（高亮显示) 设置为一级路由路径即可
      },
    },
  ],
}

export default routes
