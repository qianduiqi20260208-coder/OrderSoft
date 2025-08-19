import type { RouteRecordRaw } from 'vue-router'

function Layout() {
  return import('@/layouts/index.vue')
}

const routes: RouteRecordRaw = {
  path: '/order_manage',
  component: Layout,
  name: 'orderManageRoot',
  meta: {
    title: '工单待办',
    icon: 'i-ic:round-tab',
  },
  children: [
    {
      path: '',
      name: 'orderManage',
      component: () => import('@/views/order/order_manage.vue'),
      meta: {
        title: '工单待办',
        menu: false,
        breadcrumb: false,
        activeMenu: '/order_manage',
      },
    },
  ],
}

export default routes
