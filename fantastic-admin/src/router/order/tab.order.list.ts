import type { RouteRecordRaw } from 'vue-router'

function Layout() {
  return import('@/layouts/index.vue')
}

const routes: RouteRecordRaw = {
  path: '/order_list',
  component: Layout,
  name: 'oedertListRoot',
  meta: {
    title: '工单列表',
    icon: 'i-ic:round-tab',
  },
  children: [
    {
      path: '',
      name: 'ordertList',
      component: () => import('@/views/order/order_list.vue'),
      meta: {
        title: '工单列表',
        menu: false,
        breadcrumb: false,
        activeMenu: '/order',
      },
    },
  ],
}

export default routes
