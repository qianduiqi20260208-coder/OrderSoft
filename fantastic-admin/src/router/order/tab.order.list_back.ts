import type { RouteRecordRaw } from 'vue-router'

function Layout() {
  return import('@/layouts/index.vue')
}

const routes: RouteRecordRaw = {
  path: '/order_list_back',
  component: Layout,
  name: 'oedertListRoot_bakc',
  meta: {
    title: '工单列表_back',
    icon: 'i-ic:round-tab',
  },
  children: [
    {
      path: '',
      name: 'ordertList_back',
      component: () => import('@/views/order/order_list_back.vue'),
      meta: {
        title: '工单列表_back',
        menu: false,
        breadcrumb: false,
        activeMenu: '/order_list_back',
      },
    },
  ],
}

export default routes
