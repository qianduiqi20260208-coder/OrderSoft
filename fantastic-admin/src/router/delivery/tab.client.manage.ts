import type { RouteRecordRaw } from 'vue-router'

function Layout() {
  return import('@/layouts/index.vue')
}

const routes: RouteRecordRaw = {
  path: '/client_manage', // 一级路由路径
  component: Layout,
  name: 'Client_Manage', // 路由名称，唯一标识
  meta: {
    title: '客户管理', // 菜单或标签页显示的标题
    icon: 'i-ic:round-tab', // 菜单或标签页显示的图标
  },
  children: [
    {
      path: '', // 默认子路由，访问 /client_manage 时自动加载
      name: 'Client_Manage_Info', // 子路由名称，唯一标识 这里不重要
      component: () => import('@/views/delivery/client_manage.vue'), // 具体页面组件
      meta: {
        title: '客户管理', // 页面标题
        menu: false, // 不在菜单中显示
        breadcrumb: false, // 不在面包屑中显示
        activeMenu: '/client_manage', // 激活菜单项（高亮显示) 设置为一级路由路径即可
      },
    },
    {
      path: '/client_manage/send_detail',
      name: 'Client_Manage_SendDetail',
      component: () => import('@/views/delivery/send_detail.vue'),
      meta: {
        title: '发送详情',
        menu: false, // 不在菜单中显示
      },
    },
    {
      path: '/client_manage/auth_detail',
      name: 'Client_Manage_AuthDetail',
      component: () => import('@/views/delivery/auth_detail.vue'),
      meta: {
        title: '授权详情',
        menu: false, // 不在菜单中显示
      },
    },
  ],
}

export default routes
