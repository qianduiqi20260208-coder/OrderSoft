import api from '../index'

export default {
  // 获取工单（分页显示、可筛选）
  fetchOrderPage: (params: {
    page: number // 当前页码
    pageSize: number // 每页显示数量
    orderID?: string // 工单ID（可选，支持模糊搜索）
    type?: string // 工单类型（如问题复现、迭代等，可选）
    promoterID?: string // 发起人工号（可选）
    modelID?: string // 模型ID（可选）
    referencePriority?: string // 参考优先级（可选）
    taskPriority?: string // 任务优先级（可选）
    status?: string // 工单状态（可选）
    startDate?: string // 开始时间（筛选区间，可选）
    endDate?: string // 结束时间（筛选区间，可选）
  }) =>
    api.get('order/all', {
      // baseURL: '/mock/', // mock数据接口地址
      params, // 请求参数（用于筛选工单）
    }),

  // 获取当前用户的工单列表（支持传参：userID、role等筛选条件）
  fetchUserOrderList: (params?: {
    userID?: string // 用户ID（当前登录用户）
    role?: string // 用户角色（如管理员、普通用户等）
  }) =>
    api.get('order/list', {
      // baseURL: '/mock/', // mock数据接口地址
      params, // 请求参数（用于筛选工单）
    }),

  // 提交问题复现工单（支持FormData格式上传文件）
  submitProblemOrder: (data: FormData) =>
    api.post('order/problem', data, {
      // baseURL: '/mock/', // 需要连接真实后端请修改此处，将其注释
      headers: { 'Content-Type': 'multipart/form-data' }, // 明确指定上传格式
    }),

  // 提交版本迭代工单
  submitIterOrder: (data: {
    orderID?: string // 工单ID（新建时可为空或由后端生成）
    status: string // 工单状态（待审批）
    promoterID: string // 发起人工号（当前用户ID）
    modelID: string // 模型ID
    modelVersionID: string // 模型版本ID
    completeModelVersion: string // 期望完成模型版本（如1.0.0）
    coordinationID?: string // 协调单号（可选）
    updateNotes: string // 更新说明
    packageRequirement?: string // 封装要求（可选）
    apiChanged: string // 接口是否变化
    approverID: string // 审批人ID
    startTime: string // 工单开始时间（提交时自动生成）
  }) => api.post('order/iter', data, {
    // baseURL: '/mock/',
  }),

  // 提交交付发送工单
  submitDeliverOrder: (data: {
    orderID?: string // 工单ID（新建时可为空或由后端生成）
    status: string // 工单状态（待审批）
    promoterID: string // 发起人工号（当前用户ID）
    modelID: string // 模型ID
    modelVersionID: string // 模型版本ID
    targetCustomer: string // 目标客户
    isCAEChecked: string // 是否CAE检查（是/否）
    hasSensitiveInfo: string // 是否包含敏感信息
    approverID: string // 审批人ID
    startTime: string // 工单开始时间（提交时自动生成）
  }) => api.post('order/deliver', data, {
    // baseURL: '/mock/',
  }),

  // 提交模型迭代+交付发送工单
  submitIterDeliverOrder: (data: {
    orderID?: string // 工单ID（新建时可为空或由后端生成）
    status: string // 工单状态（待审批）
    promoterID: string // 发起人工号（当前用户ID）
    modelID: string // 模型ID
    modelVersionID: string // 模型版本ID
    completeModelVersion: string // 期望完成模型版本（如1.0.0）
    coordinationID?: string // 协调单号（可选）
    updateNotes: string // 更新说明
    packageRequirement?: string // 封装要求（可选）
    apiChanged: string // 接口是否变化
    targetCustomer: string // 目标客户
    isCAEChecked: string // 是否CAE检查（是/否）
    hasSensitiveInfo: string // 是否包含敏感信息
    approverID: string // 审批人ID
    startTime: string // 工单开始时间（提交时自动生成）
  }) => api.post('order/iter-deliver', data, {
    // baseURL: '/mock/',
  }),

  // 提交功能开发工单
  submitDevOrder: (data: {
    orderID?: string // 工单ID（新建时可为空或由后端生成）
    status: string // 工单状态（待审批）
    promoterID: string // 发起人工号（当前用户ID）
    modelID: string // 模型ID
    modelVersionID: string // 模型版本ID
    completeModelVersion: string // 期望完成模型版本（如1.0.0）
    featureDesc: string // 功能描述
    approverID: string // 审批人ID
    startTime: string // 工单开始时间（提交时自动生成）
  }) => api.post('order/dev', data, {
    // baseURL: '/mock/',
  }),

  // 提交其他工单
  submitOtherOrder: (data: {
    orderID?: string // 工单ID（新建时可为空或由后端生成）
    status: string // 工单状态（待审批）
    promoterID: string // 发起人工号（当前用户ID）
    modelID: string // 模型ID
    modelVersionID: string // 模型版本ID
    contentDesc: string // 内容描述
    approverID: string // 审批人ID
    startTime: string // 工单开始时间（提交时自动生成）
  }) => api.post('order/other', data, {
    // baseURL: '/mock/',
  }),

  // 工单审批同意接口
  approveOrder: (data: {
    orderID: string // 工单ID
    status: string // 工单状态（审批后变为待分发）
    approverID: string // 审批人ID（当前用户）
    approveTime: string // 审批通过时间（系统自动获取）
    referencePriority: string // 参考优先级（审批时设置）
    distributorID: string // 下一流程负责人ID（分发人ID）
  }) => api.post('order/approve', data, {
    // baseURL: '/mock/',
  }),

  // 审批环节拒绝接口
  rejectApproveOrder: (data: {
    orderID: string // 工单ID
    status: string // 工单状态（已退回）
    approverID: string // 审批人ID
    approveTime: string // 审批时间
    rejectReason: string // 拒绝原因
  }) => api.post('order/reject-approve', data, {
    // baseURL: '/mock/',
  }),

  // 工单分发同意接口
  distributeOrder: (data: {
    orderID: string // 工单ID
    status: string // 工单状态（分发后变为进行中）
    distributorID: string // 分发人ID
    distributeTime: string // 分发时间（系统自动获取）
    taskPriority: string // 任务优先级
    executorID: string // 执行人ID
  }) => api.post('order/distribute', data, {
    // baseURL: '/mock/',
  }),

  // 分发环节拒绝接口
  rejectDistributeOrder: (data: {
    orderID: string // 工单ID
    status: string // 工单状态（已退回）
    distributorID: string // 分发人ID
    distributeTime: string // 分发时间
    rejectReason: string // 拒绝原因
  }) => api.post('order/reject-distribute', data, {
    // baseURL: '/mock/',
  }),

  // 问题复现工单完成接口
  finishProblemOrder: (data: {
    orderID: string // 工单ID
    status: string // 工单状态（已完成）
    finishTime: string // 完成时间
    finishPhenomenon: string // 复现现象
    finishRemark: string // 备注
    executorID: string // 完成人ID
  }) => api.post('order/finish-problem', data, {
    // baseURL: '/mock/',
  }),

  // 版本迭代工单完成接口
  finishIterOrder: (data: {
    orderID: string // 工单ID
    modelID: string // 模型ID
    status: string // 工单状态（已完成）
    finishTime: string // 完成时间
    finishModelVersion: string // 升级后模型版本
    finishRemark: string // 备注
    executorID: string // 完成人ID
  }) => api.post('order/finish-iter', data, {
    // baseURL: '/mock/',
  }),

  // 交付发送工单完成接口
  finishDeliverOrder: (data: {
    orderID: string // 工单ID
    status: string // 工单状态（已完成）
    finishTime: string // 完成时间
    isEncrypted: string // 是否加密（是/否）
    finishAuthId: string // 授权ID
    finishShellNo: string[] // 外壳号
    finishRemark: string // 备注
    executorID: string // 完成人ID
  }) => api.post('order/finish-deliver', data, {
    // baseURL: '/mock/',
  }),

  // 版本迭代+交付发送工单完成接口
  finishIterDeliverOrder: (data: {
    orderID: string // 工单ID
    status: string // 工单状态（已完成）
    modelID: string // 模型ID
    finishTime: string // 完成时间
    finishModelVersion: string // 升级后模型版本
    isEncrypted: string // 是否加密（是/否）
    finishAuthId: string // 授权ID
    finishShellNo: string[] // 外壳号
    finishRemark: string // 备注
    executorID: string // 完成人ID
  }) => api.post('order/finish-iter-deliver', data, {
    // baseURL: '/mock/',
  }),

  // 功能开发工单完成接口
  finishDevOrder: (data: {
    orderID: string // 工单ID
    status: string // 工单状态（已完成）
    modelID: string // 模型ID
    modelVersion: string // 升级后模型版本
    finishTime: string // 完成时间
    finishModelVersionId: string // 完成后模型版本ID
    finishFeatureDesc: string // 完成功能描述
    executorID: string // 完成人ID
  }) => api.post('order/finish-dev', data, {
    // baseURL: '/mock/',
  }),

  // 其他工单完成接口
  finishOtherOrder: (data: {
    orderID: string // 工单ID
    status: string // 工单状态（已完成）
    finishTime: string // 完成时间
    finishRemarkOther: string // 备注（完成）
    executorID: string // 完成人ID
  }) => api.post('order/finish-other', data, {
    // baseURL: '/mock/',
  }),

  // 工单流转接口
  transferOrder: (data: {
    orderID: string // 工单ID
    executorID: string // 执行人ID（当前用户）
    transferExecutorID: string // 流转执行人ID
    transferReason: string // 流转原因
    transferTime: string // 流转时间
  }) => api.post('order/transfer', data, {
    // baseURL: '/mock/',
  }),

  // 获取模型版本列表
  fetchModelVersions: (modelId: string) =>
    api.get('model/version/list', {
      // baseURL: '/mock/',
      params: { modelId },
    }),

  // 获取审批人信息列表
  fetchApproverList: (modelId: string) =>
    api.get('approver/list', {
      // baseURL: '/mock/',
      params: { modelId },
    }),

  // 获取分发人信息列表
  fetchDistributorList: (modelId: string) =>
    api.get('distributor/list', {
      // baseURL: '/mock/',
      params: { modelId },
    }),

  // 获取流转负责人列表
  fetchTransferExecutorList: (modelId: string) =>
    api.get('transferExecutor/list', {
      // baseURL: '/mock/',
      params: { modelId },
    }),

  // 获取执行人信息列表 工单列表筛选用
  fetchAllExecutorList: () =>
    api.get('executor_all/list', {
      // baseURL: '/mock/',
    }),

  // 获取执行人信息列表
  fetchExecutorList: (modelId: string) =>
    api.get('executor/list', {
      // baseURL: '/mock/',
      params: { modelId },
    }),

  // 获取客户信息列表
  fetchCustomerList: () =>
    api.get('customer/list', {
      // baseURL: '/mock/',
    }),

  // 获取外壳号列表
  fetchShellNumbers: (targetCustomer: string) =>
    api.get('order/shell-numbers', {
      // baseURL: '/mock/',
      params: { targetCustomer },
    }),

  // 获取客户的所有授权ID列表
  fetchCustomerAuthIds: (clientName: string) =>
    api.get('order/customer-auth-ids', {
      params: { clientName },
      // baseURL: '/mock/',
    }),

  // 根据授权ID获取对应的外壳号
  fetchShellByAuthId: (authId: string, targetCustomer: string) =>
    api.get('order/shell-by-auth', {
      params: { authId, targetCustomer },
    }),

  // 根据授权ID获取外壳号列表
  fetchShellsByAuthId: (authId: string, targetCustomer: string) =>
    api.get('order/shells-by-auth', {
      params: { authId, targetCustomer },
    }),

  // 获取授权ID列表（保留原有方法以兼容其他功能）
  fetchAuthIds: (shellNumber: string, targetCustomer: string) =>
    api.get('order/auth-ids', {
      // baseURL: '/mock/',
      params: { shellNumber, targetCustomer },
    }),

  // 下载工单附件文件
  downloadTicketFile: (ticketId: number, filename: string) =>
    api.get(`files/ticket/${ticketId}/${filename}`, {
      // baseURL: '/mock/',
    }),

  // 获取工单统计数据
  fetchOrderStatisticsByCondition: (
    clientNames: string[], // 客户名数组
  ) =>
    api.get('order/statisticsAll', {
      params: {
        clientNames: clientNames.join(','), // 逗号分隔字符串
      },
    }),

  // 根据创建时填写的部分版本号获取当前数据库中已有的模型版本
  fetchUsedModelVersion: (modelId: string, completeModelVersion: string) =>
    api.get('model/version/used', {
      params: { modelId, completeModelVersion },
    }),

}
