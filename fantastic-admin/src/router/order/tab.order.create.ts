import type { RouteRecordRaw } from 'vue-router'

function Layout() {
  return import('@/layouts/index.vue')
}

const routes: RouteRecordRaw = {
  path: '/order_create',
  component: Layout,
  name: 'orderCreateRoot',
  meta: {
    title: '工单创建',
    icon: 'i-ic:round-tab',
    // auth: ['SuperUser', 'ModelEngineer'], // 所有人可见
  },
  children: [
    {
      path: '',
      name: 'orderCreate',
      component: () => import('@/views/order/order_create.vue'),
      meta: {
        title: '工单创建',
        menu: false,
        breadcrumb: false,
        activeMenu: '/order_create',
      },
    },
  ],
}

export default routes
