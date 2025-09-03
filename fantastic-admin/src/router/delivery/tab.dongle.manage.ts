import type { RouteRecordRaw } from 'vue-router'

// function Layout() {
//   return import('@/layouts/index.vue')
// }

const routes: RouteRecordRaw = {
  path: '/dongle_manage', // 一级路由路径

  // component: Layout,
  name: 'Dongle_Manage', // 路由名称，唯一标识
  meta: {
    title: '加密狗管理', // 菜单或标签页显示的标题
    icon: 'i-ic:round-tab', // 菜单或标签页显示的图标
    link: 'http://localhost:5173/key-management',
  },
  redirect: '/404',

  // children: [
  //   {
  //     path: '', // 默认子路由，访问 /dongle_manage 时自动加载
  //     name: 'Dongle_Manage_Info', // 子路由名称，唯一标识 这里不重要
  //     component: () => import('@/views/delivery/dongle_manage.vue'), // 具体页面组件
  //     meta: {
  //       title: '加密狗管理', // 页面标题
  //       menu: false, // 不在菜单中显示
  //       breadcrumb: false, // 不在面包屑中显示
  //       activeMenu: '/dongle_manage', // 激活菜单项（高亮显示) 设置为一级路由路径即可
  //       link: 'http://localhost:8888/key-management',
  //     },
  //   },
  // ],
}

export default routes
