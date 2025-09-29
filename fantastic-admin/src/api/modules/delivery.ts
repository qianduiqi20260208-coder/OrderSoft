import api from '../index'

export default {
  // -----------加密狗管理页面-----------
  // 获取加密狗列表信息
  getDongleList: () =>
    api.get('dongle/list', {
      // baseURL: '/mock/',
    }),

  // 获取特定加密狗的历史记录
  getDongleHistory: (dongleId: string) =>
    api.get(`dongle/history`, {
      // baseURL: '/mock/',
      params: { dongleId },
    }),

  // 创建加密狗
  createDongle: (data: {
    shellCode: string // 外壳号
    shellSerial: string // 序列号
  }) =>
    api.post('dongle/create', data, {
      // baseURL: '/mock/',
    }),

  // 更新加密狗信息
  updateDongle: (data: {
    dongleId: string
    shellCode: string
    shellSerial: string
  }) =>
    api.put('delivery/dongles/update', data, {
      // baseURL: '/mock/',
    }),

  // -----------客户管理页面-------------
  // 获取客户列表信息
  getClientList: () =>
    api.get('client/list', {
      // baseURL: '/mock/',
    }),

  // 新建客户
  createClient: (data: {
    clientName: string // 客户名称
    clientinfo?: string // 客户信息备注
  }) =>
    api.post('client/create', data, {
      // baseURL: '/mock/',
    }),

  // 编辑客户
  updateClient: (data: {
    originalClientName: string // 原始客户名称
    clientName: string // 新客户名称
    clientinfo?: string // 客户信息备注
  }) =>
    api.put('client/update', data, {
      // baseURL: '/mock/',
    }),

  // -----------发送详情页面-------------
  // 获取发送详情信息
  getSendDetail: (clientName: string) =>
    api.get('client/send/detail', {
      // baseURL: '/mock/',
      params: { clientName },
    }),

  // 获取发送详情（分页版本）
  getSendDetailWithPagination: (params: {
    clientName: string
    page: number
    pageSize: number
  }) =>
    api.get('client/send/detail/pagination', {
      // baseURL: '/mock/',
      params,
    }),

  // 获取发送总览（各模型最新版本）
  getSendOverview: (clientName: string) =>
    api.get('client/send/overview', {
      // baseURL: '/mock/',
      params: { clientName },
    }),

  // -----------授权详情页面-------------
  // 获取授权详情信息
  getAuthDetail: (clientName: string) =>
    api.get('client/auth/detail', {
      // baseURL: '/mock/',
      params: { clientName },
    }),

  // 获取单个外壳号的完整授权列表
  getShellAuthList: (params: {
    clientName: string
    shellNumber: string
  }) =>
    api.get('shell/auth/list', {
      // baseURL: '/mock/',
      params,
    }),

  // 交付外壳
  deliverShell: (data: {
    clientName: string // 客户名称
    shellNumber: string // 外壳号
    deviceType: string // 设备类型 (lab/IPT/FTD/FFS)
    deviceNote: string // 设备备注
  }) =>
    api.post('shell/deliver', data, {
      // baseURL: '/mock/',
    }),

  // 获取可交付的外壳号列表
  getAvailableShells: () =>
    api.get('delivery/available-shells', {
      // baseURL: '/mock/',
    }),

  // 归还外壳
  returnShell: (data: {
    clientName: string // 客户名称
    operationType: string // 操作类型
    shellNumber: string // 外壳号
    inTime: string // 入库时间
    outTime: string // 出库时间
    remark: string // 备注
  }) =>
    api.post('shell/return', data, {
      // baseURL: '/mock/',
    }),

  // 新建授权信息
  createAuth: (data: {
    clientName: string // 客户名称
    shellNumber: string // 外壳号
    authId: string // 授权ID（可选，后端生成）
    authType: string // 授权类型
    startDate: string // 授权开始日期
    endDate: string // 授权结束日期
    authNote: string // 授权备注
  }) =>
    api.post('auth/create', data, {
      // baseURL: '/mock/',
    }),

  // 更新外壳号信息
  updateShellInfo: (data: {
    clientName: string // 客户名称
    shellNumber: string // 外壳号
    deviceType: string // 设备类型
    deviceNote: string // 设备备注
  }) =>
    api.put('shell/update', data, {
      // baseURL: '/mock/',
    }),

  // 批量更新授权截止日期
  batchUpdateAuthEndDates: (data: {
    clientName: string // 客户名称
    // 需要更新的外壳号和授权ID列表
    changes: Array<{
      shellNumber: string // 外壳号
      authId: string // 授权ID
      newEndDate: string // 新的授权截止日期
    }>
  }) =>
    api.put('auth/batch-update', data, {
      // baseURL: '/mock/',
    }),

  // 获取客户模型版本历史信息
  getCustomerModelVersionHistory: (clientName: string) => {
    console.log(`API请求: 获取客户 ${clientName} 的版本历史`)
    return api.get('client/model-version-history', {
      params: { clientName },
    }).then(response => {
      console.log(`API响应: 客户 ${clientName} 的版本历史`, response)
      return response
    }).catch(error => {
      console.error(`API错误: 客户 ${clientName} 的版本历史`, error)
      throw error
    })
  },
}
