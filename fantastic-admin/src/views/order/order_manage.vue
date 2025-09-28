<route lang="yaml">
meta:
  title: 工单列表
  # enabled: false
</route>

<script setup lang="ts">
import { ElMessage } from 'element-plus'
import { onMounted, ref } from 'vue'
import orderApi from '@/api/modules/order'
import { useUserStore } from '@/store/modules/user'

// -----------------数据结构-----------------
// 完成工单流转信息
interface TransferInfo {
  transferExecutorID: string // 流转执行人ID
  transferCreatorID: string // 流转创建人ID
  transferReason: string
  transferTime: string
}

// 加密环节流转信息
interface TransferInfo_Encrypted {
  transferExecutorID: string // 流转执行人ID
  transferCreatorID: string // 流转创建人ID
  transferReason: string
  transferTime: string
}

// 发送环节流转信息
interface TransferInfo_Delivery {
  transferExecutorID: string // 流转执行人ID
  transferCreatorID: string // 流转创建人ID
  transferReason: string
  transferTime: string
}

// 封装环节流转信息
interface TransferInfo_Version {
  transferExecutorID: string // 流转执行人ID
  transferCreatorID: string // 流转创建人ID
  transferReason: string
  transferTime: string
}

// 封装环节信息
interface Version_Info {
  // 版本迭代类（完成）
  finishModelVersion: string // 升级后模型版本
  versionRemark?: string // 封装环节备注
}

// 加密环节信息
interface Encrypted_Info {
  // 交付发送类（完成）
  isEncrypted: string // 是否加密（是/否）
  finishAuthId?: string // 授权ID（数字）
  finishShellNo?: string // 外壳号（字母+数字）
  encryptedRemark?: string // 加密环节备注
}

// 工单数据结构定义，统一用 interface 进行类型约束
interface OrderItem {
  // 工单类型
  type: string // 工单类型
  // 工单状态
  status: '草稿' | '待审批' | '待分发' | '进行中' | '已完成' | '已退回'

  statusTodo?: string // 待封装 0 待加密 1 待发送 2
  // statusTodo?: '待封装' | '待加密' | '待发送' // 待封装 0 待加密 1 待发送 2
  // 参考优先级
  referencePriority?: '紧急' | '一般' | ''
  // 任务优先级
  taskPriority?: '紧急' | '一般' | ''

  orderID: string // 工单ID
  modelID: string // 模型ID
  modelVersionID: string // 模型版本ID
  completeModelVersion?: string // 完成模型版本(创建时填写)
  usedModelVersions?: string[] // 已使用的模型版本列表
  promoterID: string // 发起人ID
  startTime: string // 发起时间
  approverID?: string // 审批人ID
  distributorID?: string // 分发人ID
  toDispatcherName?: string // 分发人下一流程的人
  approveTime?: string // 审批时间
  distributeTime?: string // 分发时间
  rejectReason?: string // 拒绝原因
  executorID?: string // 执行人ID
  finishTime?: string // 完成时间
  targetDeliveryTime?: string // 预计发送时间 工单审批时填写

  // ----------问题复现工单（创建）----------
  coordinationID?: string // 协调单号
  description?: string // 复现内容描述
  // 后端返回的文件相关字段
  fileName?: string // 文件名
  fileUrl?: string // 文件URL
  hasAttachment?: boolean // 是否有附件
  // 前端使用的文件数组（转换后的格式）
  files?: Array<{ fileName: string, fileUrl: string }> // 附件列表

  // ----------版本迭代类（创建）----------
  updateNotes?: string // 更新内容
  packageRequirement?: string // 封装要求
  apiChanged?: string // 接口是否变化（是/否）

  // ----------交付发送类（创建）----------
  targetCustomer?: string // 目标客户
  isCAEChecked?: string // 是否通过CAE平台验证（是/否）
  hasSensitiveInfo?: string // 当前版本是否包含敏感信息（是/否）

  // ----------功能开发类（创建）----------
  featureDesc?: string // 功能描述

  // ----------其他类（创建）----------
  contentDesc?: string // 内容描述

  // ----------完成工单相关字段（不可与创建复用）----------
  finishRemark?: string // 备注

  // 问题复现类（完成）
  finishPhenomenon?: string // 复现现象

  // 版本迭代类（完成）
  finishModelVersion?: string // 升级后模型版本

  // 交付发送类（完成）
  isEncrypted?: string // 是否加密（是/否）
  finishAuthId?: string // 授权ID（数字）
  finishShellNo?: string[] // 外壳号（字母+数字）
  authorizationId_list?: string[] // 授权ID主键数组

  // 功能开发类（完成）
  // finishModelVersionId?: string // 完成后模型版本ID
  finishFeatureDesc?: string // 完成功能描述

  // 其他类（完成）
  finishRemarkOther?: string // 备注（完成）

  // 流转相关内容
  transfers?: TransferInfo[] // 多次流转记录
  transfers_Encrypted?: TransferInfo_Encrypted[] // 加密环节流转记录
  transfers_Delivery?: TransferInfo_Delivery[] // 发送环节流转记录
  transfers_Version?: TransferInfo_Version[] // 封装环节流转记录
  versionInfo?: Version_Info // 封装环节信息
  encryptedInfo?: Encrypted_Info // 加密环节信息

  sendRemark?: string // 发送备注
  encryptedRemark?: string // 加密备注
  packageRemark?: string // 封装备注

  packageExecutorID?: string // 封装人ID
  encryptedExecutorID?: string // 加密人ID
  sendExecutorID?: string // 发送人ID

  packageAt?: string // 封装时间
  encryptedAt?: string // 加密时间
  sendAt?: string // 发送时间

  currentExecutor?: string // 当前步骤执行人

  // 前端本次编辑用（不提交时不影响历史）
  transferReasonEdit?: string
  transferExecutorIDEdit?: string
  transferTimeEdit?: string

  encryptedExecutorIDEdit?: string // 加密人ID 编辑用

  finishModelVersionNumber?: string
  finishModelVersionLetter?: string
  createRemark?: string
  targetPlatform?: string
}

const leaderPriority = ref<'' | '紧急' | '一般'>('')
const loading = ref(false)
const userOrders = ref<OrderItem[]>([])// 工单列表数据，类型约束为 OrderItem 数组
const expandedMap = ref<Record<string, boolean>>({})// 每个工单的展开状态
const userStore = useUserStore()
const currentUserId = userStore.account // 获取当前登录用户ID
const currentRole = userStore.role // 角色字段需在 userStore 中维护

// 处理退回逻辑
const rejectDialogVisible = ref(false) // 控制拒绝原因弹窗显示/隐藏
const rejectReasonInput = ref('') // 存储弹窗中填写的拒绝原因内容
const rejectOrder = ref<OrderItem | null>(null) // 当前被拒绝的工单对象

// 批量审批功能
const selectedOrderIds = ref<string[]>([]) // 存储所有已选中的工单ID

// 批量审批弹窗相关
const batchApproveDialogVisible = ref(false) // 控制批量审批弹窗显示/隐藏
const batchLeaderPriority = ref<'' | '紧急' | '一般'>('') // 批量审批时统一设置参考优先级
const batchDistributorID = ref('') // 批量审批时统一设置下一流程负责人ID（分发人ID）

// 批量分发弹窗相关
const batchDistributeDialogVisible = ref(false) // 控制批量分发弹窗显示/隐藏
const batchTaskPriority = ref<'' | '紧急' | '一般'>('') // 批量分发时统一设置任务优先级
const batchExecutorID = ref('') // 批量分发时统一设置下一流程负责人ID（执行人ID）

// Tab状态管理
const activeTabMap = ref<Record<string, string>>({}) // 存储每个工单当前激活的Tab

// 预计发送时间快捷选项
const deliveryTimeShortcuts = [
  {
    text: '今天',
    value: () => {
      const date = new Date()
      date.setTime(date.getTime())
      return date
    }
  },
  {
    text: '明天',
    value: () => {
      const date = new Date()
      date.setTime(date.getTime() + 3600 * 1000 * 24)
      return date
    }
  },
  {
    text: '后天',
    value: () => {
      const date = new Date()
      date.setTime(date.getTime() + 3600 * 1000 * 24 * 2)
      return date
    }
  },
  {
    text: '三天后',
    value: () => {
      const date = new Date()
      date.setTime(date.getTime() + 3600 * 1000 * 24 * 3)
      return date
    }
  },
  {
    text: '一周后',
    value: () => {
      const date = new Date()
      date.setTime(date.getTime() + 3600 * 1000 * 24 * 7)
      return date
    }
  }
]

// -----------事件处理函数--------------
function expandOrder(orderId: string, expand: boolean) {
  expandedMap.value[orderId] = expand

  if (expand) {
    const idx = userOrders.value.findIndex(o => o.orderID === orderId)
    const order = userOrders.value[idx]
    if (order && order.status === '进行中' && order.completeModelVersion) {
      orderApi.fetchUsedModelVersion(order.modelID, order.completeModelVersion)
        .then((res) => {
          if (res?.data?.list) {
            userOrders.value[idx].usedModelVersions = res.data.list || []
            console.warn(userOrders.value[idx].usedModelVersions)
            // 强制刷新
            userOrders.value = [...userOrders.value]
          }
        })
        .catch((err) => {
          ElMessage.error('获取新版本号失败')
          console.error(err)
        })
    }
  }
}

// 处理任务审批通过逻辑
async function handleApprove(order: OrderItem) {
  // 验证：交付发送和版本迭代+交付发送类工单必须填写预计发送时间
  if (['交付发送', '版本迭代+交付发送'].includes(order.type) && !order.targetDeliveryTime) {
    ElMessage.error('请先填写预计发送时间')
    return
  }

  // ------------页面设置------------
  order.referencePriority = leaderPriority.value // 设置参考优先级
  order.status = '待分发' // 工单状态修改为待分发

  order.approverID = currentUserId // 设置审批领导ID
  const now = new Date()
  // 设置审批时间
  order.approveTime = `${now.getFullYear()}-${(now.getMonth() + 1).toString().padStart(2, '0')}-${now.getDate().toString().padStart(2, '0')} ${now.getHours().toString().padStart(2, '0')}:${now.getMinutes().toString().padStart(2, '0')}:${now.getSeconds().toString().padStart(2, '0')}`
  // 清空优先级选择
  leaderPriority.value = ''
  // 调用后端接口保存审批结果
  const res = await orderApi.approveOrder({
    orderID: order.orderID, // 工单ID
    status: order.status, // 工单状态（审批后变为待分发）
    orderType: order.type, // 工单类型
    approverID: String(order.approverID ?? ''), // 审批人ID（当前用户）强制转换为String后发送后端
    approveTime: order.approveTime, // 审批通过时间（系统自动获取）
    referencePriority: order.referencePriority, // 参考优先级（审批时设置）
    targetDeliveryTime: order.targetDeliveryTime ?? '', // 预计发送时间
    distributorID: String(order.distributorID ?? ''), // 下一流程负责人ID（分发人ID）强制转换为String后发送后端
  })
  // 显示后端返回的 message
  if (res?.data?.message) {
    ElMessage.success(res.data.message)
  }
  // 刷新工单列表
  fetchUserOrders()
}

let statusBeforeReject: string | undefined // 记录拒绝前的工单状态

function handleReject(order: OrderItem) {
  // 打开拒绝弹窗，并记录当前工单状态
  rejectOrder.value = { ...order }
  statusBeforeReject = order.status // 记录拒绝前的状态
  rejectReasonInput.value = ''
  rejectDialogVisible.value = true
}

async function confirmReject() {
  // 确认拒绝时，根据工单状态区分审批环节和分发环节
  if (!rejectOrder.value) {
    return
  }
  rejectOrder.value.status = '已退回'
  rejectOrder.value.rejectReason = rejectReasonInput.value
  let res
  if (statusBeforeReject === '待审批' || statusBeforeReject === undefined) {
    // 审批环节拒绝：设置审批人和审批时间，调用审批拒绝接口
    rejectOrder.value.approverID = currentUserId
    rejectOrder.value.approveTime = new Date().toISOString().slice(0, 19).replace('T', ' ')
    res = await orderApi.rejectApproveOrder({
      orderID: rejectOrder.value.orderID, // 工单ID
      status: rejectOrder.value.status, // 工单状态（已退回）
      approverID: rejectOrder.value.approverID ?? '', // 审批人ID
      approveTime: rejectOrder.value.approveTime, // 审批时间
      rejectReason: rejectOrder.value.rejectReason, // 拒绝原因
    })
  }
  else if (statusBeforeReject === '待分发') {
    // 分发环节拒绝：设置分发人和分发时间，调用分发拒绝接口
    rejectOrder.value.distributorID = currentUserId
    rejectOrder.value.distributeTime = new Date().toISOString().slice(0, 19).replace('T', ' ')
    res = await orderApi.rejectDistributeOrder({
      orderID: rejectOrder.value.orderID, // 工单ID
      status: rejectOrder.value.status, // 工单状态（已退回）
      distributorID: rejectOrder.value.distributorID ?? '', // 分发人ID
      distributeTime: rejectOrder.value.distributeTime, // 分发时间
      rejectReason: rejectOrder.value.rejectReason, // 拒绝原因
    })
  }
  rejectDialogVisible.value = false
  // 显示后端返回的 message
  if (res?.data?.message) {
    ElMessage.success(res.data.message)
  }
  fetchUserOrders()
}

// 处理任务分发逻辑
async function handleDistribute(order: OrderItem) {
  // ------------页面设置------------
  // 设置任务优先级
  order.taskPriority = leaderPriority.value
  // 工单状态修改为进行中
  order.status = '进行中'
  // 设置分发领导ID和分发时间
  order.distributorID = currentUserId // 当前登录用户ID
  order.distributeTime = new Date().toISOString().slice(0, 19).replace('T', ' ')
  // 清空优先级选择
  leaderPriority.value = ''
  // ------------后端设置------------
  // 调用后端接口保存分发结果
  const res = await orderApi.distributeOrder({
    orderID: order.orderID, // 工单ID
    orderType: order.type, // 工单类型
    status: order.status, // 工单状态（分发后变为进行中）
    distributorID: String(order.distributorID ?? ''), // 分发人ID
    distributeTime: order.distributeTime, // 分发时间（系统自动获取）
    taskPriority: order.taskPriority, // 任务优先级
    executorID: String(order.toDispatcherName ?? ''), // 执行人ID
  })
  // 显示后端返回的 message
  if (res?.data?.message) {
    ElMessage.success(res.data.message)
  }
  // 刷新工单列表
  fetchUserOrders()
}

// ------------二次弹窗确认---------------------
// 二次确认弹窗相关
const finishConfirmDialogVisible = ref(false) // 控制完成工单确认弹窗显示/隐藏
const confirmOrder = ref<OrderItem | null>(null) // 当前要确认的工单对象

// 处理完成工单按钮点击事件（显示确认弹窗）
function handleFinishOrderClick(order: OrderItem) {
  confirmOrder.value = { ...order }
  finishConfirmDialogVisible.value = true
}

// 确认提交完成工单
async function confirmFinishOrder() {
  if (!confirmOrder.value) {
    return
  }

  // 校验：版本迭代、版本迭代+交付发送、功能开发工单，填写的完整版本号不能与已使用版本重复
  if (confirmOrder.value.statusTodo === '待封装') {
    const needCheckTypes = ['版本迭代', '版本迭代+交付发送', '功能开发']
    if (
      needCheckTypes.includes(confirmOrder.value.type)
      && confirmOrder.value.finishModelVersion
      && Array.isArray(confirmOrder.value.usedModelVersions)
      && confirmOrder.value.usedModelVersions.includes(confirmOrder.value.finishModelVersion)
    ) {
      ElMessage.error('填写的完整版本号已被使用，请输入未使用的新版本号')
      return
    }
  }

  // 如果选择不加密，清空外壳号和授权ID字段
  if (confirmOrder.value.isEncrypted === '否') {
    confirmOrder.value.finishShellNo = []
    confirmOrder.value.finishAuthId = ''
  }

  // 设置状态为已完成，记录完成时间
  // confirmOrder.value.status = '已完成'
  confirmOrder.value.finishTime = new Date().toISOString().slice(0, 19).replace('T', ' ')
  confirmOrder.value.executorID = currentUserId // 增加完成人ID（当前用户）

  // 根据工单类型分别调用后端接口
  let res
  if (confirmOrder.value.type === '问题复现') {
    confirmOrder.value.status = '已完成'
    res = await orderApi.finishProblemOrder({
      orderID: confirmOrder.value.orderID, // 工单ID
      status: confirmOrder.value.status, // 工单状态
      finishTime: confirmOrder.value.finishTime, // 完成时间
      finishPhenomenon: confirmOrder.value.finishPhenomenon ?? '', // 复现现象
      finishRemark: confirmOrder.value.finishRemark ?? '', // 完成备注
      executorID: confirmOrder.value.executorID ?? '', // 执行人ID
    })
  }
  else if (confirmOrder.value.type === '版本迭代') {
    if (confirmOrder.value.statusTodo === '待封装') {
      confirmOrder.value.status = '已完成'
      res = await orderApi.finishpackageOrder({
        orderID: confirmOrder.value.orderID,
        orderType: confirmOrder.value.type,
        modelID: confirmOrder.value.modelID,
        status: confirmOrder.value.status,
        finishTime: confirmOrder.value.finishTime,
        finishModelVersion: confirmOrder.value.finishModelVersion ?? '',
        packageRemark: confirmOrder.value.packageRemark ?? '',
        executorID: confirmOrder.value.executorID ?? '',
      })
    }
  }
  else if (confirmOrder.value.type === '交付发送') {
    // 当前工单处于待加密状态
    if (confirmOrder.value.statusTodo === '待加密') {
      confirmOrder.value.status = '进行中'
      res = await orderApi.finishEncryptOrder({
        orderID: confirmOrder.value.orderID, // 工单id
        status: confirmOrder.value.status, // 工单状态
        orderType: confirmOrder.value.type,
        finishTime: confirmOrder.value.finishTime, // 此步骤完成时间
        isEncrypted: confirmOrder.value.isEncrypted ?? '', // 是否加密
        finishAuthId: confirmOrder.value.finishAuthId ?? '', // 授权id
        finishShellNo: confirmOrder.value.finishShellNo ?? [], // 外壳号列表
        authorizationId_list: confirmOrder.value.authorizationId_list ?? [], // 授权id主键列表
        encryptedRemark: confirmOrder.value.encryptedRemark ?? '', // 加密备注
        executorID: confirmOrder.value.executorID ?? '', // 执行人id
        sendExecutorID: confirmOrder.value.sendExecutorID ?? '', // 发送人id
      })
    }
    // 当前工单处于待发送状态
    else if (confirmOrder.value.statusTodo === '待发送') {
      confirmOrder.value.status = '已完成'
      res = await orderApi.finishSendOrder({
        orderID: confirmOrder.value.orderID, // 工单id
        status: confirmOrder.value.status, // 工单状态
        orderType: confirmOrder.value.type,
        finishTime: confirmOrder.value.finishTime, // 此步骤完成时间
        sendRemark: confirmOrder.value.sendRemark ?? '', // 发送备注
        executorID: confirmOrder.value.executorID ?? '', // 执行人id
      })
    }
  }
  else if (confirmOrder.value.type === '版本迭代+交付发送') {
    if (confirmOrder.value.statusTodo === '待封装') {
      confirmOrder.value.status = '进行中'
      res = await orderApi.finishpackageOrder({
        orderID: confirmOrder.value.orderID,
        orderType: confirmOrder.value.type,
        modelID: confirmOrder.value.modelID,
        status: confirmOrder.value.status,
        finishTime: confirmOrder.value.finishTime,
        finishModelVersion: confirmOrder.value.finishModelVersion ?? '',
        packageRemark: confirmOrder.value.packageRemark ?? '',
        executorID: confirmOrder.value.executorID ?? '',
        encryptedExecutorID: confirmOrder.value.encryptedExecutorIDEdit ?? '', // 选择下一步加密人id
      })
    }
    else if (confirmOrder.value.statusTodo === '待加密') {
      confirmOrder.value.status = '进行中'
      res = await orderApi.finishEncryptOrder({
        orderID: confirmOrder.value.orderID, // 工单id
        status: confirmOrder.value.status, // 工单状态
        orderType: confirmOrder.value.type,
        finishTime: confirmOrder.value.finishTime, // 此步骤完成时间
        isEncrypted: confirmOrder.value.isEncrypted ?? '', // 是否加密
        finishAuthId: confirmOrder.value.finishAuthId ?? '', // 授权id
        finishShellNo: confirmOrder.value.finishShellNo ?? [], // 外壳号列表
        authorizationId_list: confirmOrder.value.authorizationId_list ?? [], // 授权id主键列表
        encryptedRemark: confirmOrder.value.encryptedRemark ?? '', // 加密备注
        executorID: confirmOrder.value.executorID ?? '', // 执行人id
        sendExecutorID: confirmOrder.value.sendExecutorID ?? '', // 发送人id
      })
    }
    else if (confirmOrder.value.statusTodo === '待发送') {
      confirmOrder.value.status = '已完成'
      res = await orderApi.finishSendOrder({
        orderID: confirmOrder.value.orderID, // 工单id
        status: confirmOrder.value.status, // 工单状态
        orderType: confirmOrder.value.type,
        finishTime: confirmOrder.value.finishTime, // 此步骤完成时间
        sendRemark: confirmOrder.value.sendRemark ?? '', // 发送备注
        executorID: confirmOrder.value.executorID ?? '', // 执行人id
      })
    }
  }
  else if (confirmOrder.value.type === '功能开发') {
    res = await orderApi.finishDevOrder({
      orderID: confirmOrder.value.orderID,
      status: confirmOrder.value.status,
      modelID: confirmOrder.value.modelID,
      modelVersion: confirmOrder.value.modelVersionID,
      finishTime: confirmOrder.value.finishTime,
      finishModelVersionId: confirmOrder.value.finishModelVersion ?? '',
      finishFeatureDesc: confirmOrder.value.finishFeatureDesc ?? '',
      executorID: confirmOrder.value.executorID ?? '',
    })
  }
  else if (confirmOrder.value.type === '其他') {
    res = await orderApi.finishOtherOrder({
      orderID: confirmOrder.value.orderID,
      status: confirmOrder.value.status,
      finishTime: confirmOrder.value.finishTime,
      finishRemarkOther: confirmOrder.value.finishRemarkOther ?? '',
      executorID: confirmOrder.value.executorID ?? '',
    })
  }

  // 关闭弹窗
  finishConfirmDialogVisible.value = false
  confirmOrder.value = null

  // 显示后端返回的 message
  if (res?.data?.message) {
    ElMessage.success(res.data.message)
  }

  // 刷新工单列表
  fetchUserOrders()
}

// 判断完成工单内容是否填写完整
function isFinishOrderFilled(order: OrderItem): boolean {
  switch (order.type) {
    case '问题复现':
      return !!order.finishPhenomenon
    case '版本迭代':
      return !!order.finishModelVersion
    case '交付发送':
      // 修改逻辑：根据是否加密来判断验证条件
      if (order.isEncrypted === '是') {
        // 选择加密：需要填写授权id以及下一步发送人
        return !!order.finishAuthId && !!order.isEncrypted && !!order.sendExecutorID
      }
      else if (order.isEncrypted === '否') {
        // 选择不加密，需要选择下一步的发送人
        return !!order.isEncrypted && !!order.sendExecutorID
      }
      else {
        // 未选择是否加密
        return false
      }
    case '版本迭代+交付发送':
      // 同样修改版本迭代+交付发送的逻辑
      if (order.isEncrypted === '是') {
        // 选择加密：需要填写模型版本和授权ID
        return !!order.finishAuthId && !!order.isEncrypted && !!order.sendExecutorID
      }
      else if (order.isEncrypted === '否') {
        // 选择不加密：只需要填写模型版本
        return !!order.isEncrypted && !!order.sendExecutorID
      }
      else {
        // 未选择是否加密
        return false
      }
    case '功能开发':
      return !!order.finishModelVersion
    case '其他':
      return !!order.finishRemarkOther
    default:
      return false
  }
}

// ============= Tab 相关函数 =============
// 获取工单Tab列表
function getOrderTabs(order: OrderItem) {
  const tabs = []

  // 初始化工单 - 所有工单都有
  tabs.push({
    key: 'initiate',
    label: '任务发起',
    icon: '📋',
    status: 'completed',
    responsible: order.promoterID
  })

  // 审批工单 - 待审批、待分发、进行中、已完成状态
  if (['待审批', '待分发', '进行中', '已完成'].includes(order.status)) {
    tabs.push({
      key: 'approve',
      label: '任务审批',
      icon: '✅',
      status: order.status === '待审批' ? 'in_progress' : 'completed',
      responsible: order.approverID
    })
  }

  // 分发工单 - 待分发、进行中、已完成状态
  if (['待分发', '进行中', '已完成'].includes(order.status)) {
    tabs.push({
      key: 'distribute',
      label: '任务分发',
      icon: '📤',
      status: order.status === '待分发' ? 'in_progress' : 'completed',
      responsible: order.distributorID
    })
  }

  // 封装工单 - 版本迭代类（进行中、已完成状态且待封装）
  if (['进行中', '已完成'].includes(order.status) &&
    ['版本迭代', '版本迭代+交付发送'].includes(order.type) &&
    ['待封装', '待加密', '待发送'].includes(order.statusTodo ?? '')) {
    tabs.push({
      key: 'package',
      label: '封装工单',
      icon: '📦',
      status: order.status === '进行中' && order.statusTodo === '待封装' ? 'in_progress' : 'completed',
      responsible: order.packageExecutorID || order.executorID
    })
  }

  // 加密工单 - 版本迭代+交付发送类
  if (['进行中', '已完成'].includes(order.status) &&
    (order.type === '版本迭代+交付发送' || order.type === '交付发送') &&
    ['待加密', '待发送'].includes(order.statusTodo ?? '')) {
    tabs.push({
      key: 'encrypt',
      label: '加密工单',
      icon: '🔐',
      status: order.status === '进行中' && order.statusTodo === '待加密' ? 'in_progress' : 'completed',
      responsible: order.encryptedExecutorID
    })
  }

  // 发送工单 - 交付发送和版本迭代+交付发送类
  if (['进行中', '已完成'].includes(order.status) &&
    ['交付发送', '版本迭代+交付发送'].includes(order.type) &&
    order.statusTodo === '待发送') {
    tabs.push({
      key: 'send',
      label: '发送工单',
      icon: '📨',
      status: order.status === '进行中' && order.statusTodo === '待发送' ? 'in_progress' : 'completed',
      responsible: order.sendExecutorID
    })
  }

  // 完成工单 - 问题复现、功能开发、其他类
  if (['进行中', '已完成'].includes(order.status) &&
    ['问题复现', '功能开发', '其他'].includes(order.type)) {
    tabs.push({
      key: 'complete',
      label: '完成工单',
      icon: '✅',
      status: order.status === '进行中' ? 'in_progress' : 'completed',
      responsible: order.executorID
    })
  }

  return tabs
}

// 切换Tab
function switchTab(orderId: string, tabKey: string) {
  activeTabMap.value[orderId] = tabKey
}

// 获取当前激活的Tab
function getActiveTab(orderId: string) {
  if (!activeTabMap.value[orderId]) {
    // 对于待办工单，显示最后一步的tab
    const order = userOrders.value.find(o => o.orderID === orderId)
    if (order) {
      const tabs = getOrderTabs(order)
      // 显示最后一个tab（最后一步）
      activeTabMap.value[orderId] = tabs[tabs.length - 1]?.key || 'initiate'
    }
  }
  return activeTabMap.value[orderId] || 'initiate'
}

// 搜索/筛选请求，向后端请求工单列表数据
async function fetchUserOrders() {
  loading.value = true // 开始加载，显示骨架屏
  try {
    // 构造请求参数，包含分页和所有筛选条件
    const params = {
      userID: currentUserId,
      role: currentRole,
    }
    // 调用后端接口，获取数据
    const res = await orderApi.fetchUserOrderList(params)
    console.warn(res)

    // 处理返回的数据，将单个文件转换为文件数组
    const orders = res.data.list || []
    userOrders.value = orders.map((order: any) => {
      // 数据映射处理 - 将后端字段映射到前端期望的字段
      const mappedOrder: OrderItem = {
        // 基础字段
        orderID: order.workOrderId || '', // 工单ID ===================
        type: order.workOrderType || '其他', // 工单类型 ======================
        status: order.workOrderStatus || '草稿', // 工单状态 ========================
        statusTodo: order.statusTodo, // 流程状态====================
        referencePriority: order.priority || '', // 参考优先级 =======================
        taskPriority: order.taskPriority || '', // 任务优先级 =====================
        modelID: order.model || '', // 模型ID =====================
        modelVersionID: order.modelVersion || '', // 模型版本ID ======================
        promoterID: order.creatorName || '', // 创建人ID ======================
        startTime: order.createdAt || '', // 创建时间 =======================
        completeModelVersion: order.versionIteration?.matlabVersion || order.packageSend?.matlabVersion || order.functionDevelopment?.matlabVersion || '', // matlab版本号 ====================

        // 审批分发相关
        approverID: order.approverName || '', // 审批人 ========================
        distributorID: order.dispatcherName || '', // 分发人 =====================
        toDispatcherName: order.toDispatcherName || '', // 分发人下一流程的人 =================
        executorID: order.executorName || '', // 执行人 ========================
        approveTime: order.approvedAt || '', // 审批时间 ========================
        distributeTime: order.dispatchedAt || '', // 分发时间 =================

        // 工单类型特定字段 - 创建阶段

        coordinationID: order.issueReproduction?.coordinationId || order.versionIteration?.coordinationId || order.packageSend?.coordinationId || '', // 协调单号 ======================
        description: order.issueReproduction?.description || '', // 问题描述 ======================
        // 先初始化为空数组，后面会处理文件
        files: [],
        fileName: order.issueReproduction?.fileName || '', // 后端返回的文件名
        fileUrl: order.issueReproduction?.referenceFile || '', // 后端返回的文件URL
        hasAttachment: order.issueReproduction?.hasAttachment || false, // 是否有附件-------------------

        updateNotes: order.versionIteration?.updateContent || order.packageSend?.updateContent || '', // 版本更新内容说明 =========================
        packageRequirement: order.versionIteration?.packagingRequirements || order.packageSend?.packagingRequirements || '', // 封装要求 =======================
        apiChanged: order.versionIteration?.interfaceChanged || order.packageSend?.interfaceChanged || '', // 接口是否变化 =====================

        targetCustomer: order.deliverySend?.targetCustomer || order.packageSend?.targetCustomer || '', // 目标客户名称 ======================
        isCAEChecked: order.deliverySend?.validatedByCae || order.packageSend?.validatedByCae || '', // 是否通过CAE ====================
        hasSensitiveInfo: order.deliverySend?.sensitiveInfo || order.packageSend?.sensitiveInfo || '', // 是否包含敏感信息 ========================
        // 预计发送时间 =================
        targetDeliveryTime: (
          order.deliverySend?.targetDeliveryTime
            ? order.deliverySend.targetDeliveryTime.split(' ')[0]
            : (order.packageSend?.targetDeliveryTime
              ? order.packageSend.targetDeliveryTime.split(' ')[0]
              : '')
        ),

        featureDesc: order.functionDevelopment?.descriptionCreate || '', // 功能描述 =====================

        contentDesc: order.otherWorkOrder?.description || '', // 内容描述====================

        // 工单类型特定字段 - 完成阶段
        finishRemark: order.issueReproduction?.remarks || order.versionIteration?.remarks || order.deliverySend?.remarks || order.packageSend?.remarks || '', // 完成时备注 ======================
        finishPhenomenon: order.issueReproduction?.phenomenon || '', // 复现现象描述 ==================
        finishModelVersion: order.versionIteration?.newModelVersionId || order.executorInfo?.packageUpdateVersion || order.functionDevelopment?.newModelVersionId || '', // 升级后模型版本 ====================
        isEncrypted: order.deliverySend?.isEncrypted || order.packageSend?.isEncrypted || order.packageSend?.isEncrypted || '', // 是否加密 =========================
        finishAuthId: order.deliverySend?.authorizationId || order.packageSend?.productAuthorizationId || '', // 授权ID ======================
        // shellCode 是以逗号分割的字符串 需要转成数组
        finishShellNo: order.deliverySend?.shellCode?.split(',') || [], // 外壳号 =======================
        finishFeatureDesc: order.functionDevelopment?.descriptionCompleted || '', // 完成后功能描述 =========================

        finishRemarkOther: order.otherWorkOrder?.remarks || '', // 其他类工单完成时备注===================

        // 流转记录
        transfers: order.transferInfo || [], // 流转记录 ====================

        transfers_Encrypted: order.transferInfoEncrypted || [], // 加密环节流转记录 =====================
        transfers_Delivery: order.transferInfoDelivery || [], // 发送环节流转记录 ===================
        transfers_Version: order.transferInfoVersion || [], // 封装环节流转记录 =======================

        sendRemark: order.sendRemark || '', // 发送备注
        encryptedRemark: order.encryptedRemark || order.deliverySend?.remarks || '', // 加密备注
        packageRemark: order.packageRemark || '', // 封装备注

        packageExecutorID: order.executorInfo?.packageCreateName || '', // 封装人ID
        sendExecutorID: order.executorInfo?.senderName || '', // 发送人ID
        encryptedExecutorID: order.executorInfo?.encryptorName || '', // 加密人ID

        currentExecutor: order.executorName || '', // 当前流程负责人

        packageAt: order.executorInfo?.packageCreateAt || '', // 封装时间
        encryptedAt: order.executorInfo?.encryptorTime || '', // 加密时间
        sendAt: order.executorInfo?.senderTime || '', // 发送时间

        versionInfo: order.Version_Info || '', // 封装环节信息
        encryptedInfo: order.Encrypted_Info || '', // 加密环节信息
        createRemark: order.createRemark || '', // 创建备注
        targetPlatform: order.versionIteration?.targetPlatform || order.functionDevelopment?.targetPlatform || '',
      }

      // 处理文件信息，转换为 files 数组格式
      if (mappedOrder.hasAttachment && mappedOrder.fileName && mappedOrder.fileUrl) {
        mappedOrder.files = [{
          fileName: mappedOrder.fileName,
          fileUrl: mappedOrder.fileUrl,
        }]
      }
      else {
        mappedOrder.files = []
      }

      return mappedOrder
    })

    userOrders.value = userOrders.value.map((order) => {
      if (order.type === '直接封装+发送') {
        return { ...order, type: '版本迭代+交付发送' }
      }
      return order
    })
    console.warn(userOrders.value)
  }
  finally {
    loading.value = false // 加载结束，隐藏骨架屏
  }
}

// -----------批量审批功能----------------
// 打开批量审批弹窗
function openBatchApproveDialog() {
  batchLeaderPriority.value = ''
  batchDistributorID.value = ''

  // 如果选择的工单不全是待审批状态的工单 报错
  if (selectedOrderIds.value.some((id) => {
    const order = userOrders.value.find(o => o.orderID === id)
    return !order || order.status !== '待审批'
  })) {
    ElMessage.warning('请选择待审批状态的工单')
    return
  }

  // 如果没有选择任何工单，则自动选中所有待审批工单
  if (selectedOrderIds.value.length === 0) {
    selectedOrderIds.value = userOrders.value
      .filter(order => order.status === '待审批')
      .map(order => order.orderID)
  }
  console.warn(selectedOrderIds.value)
  batchApproveDialogVisible.value = true
}

// 打开批量分发弹窗
function openBatchDistributeDialog() {
  batchTaskPriority.value = ''
  batchExecutorID.value = ''

  // 如果选择的工单不全是待分发状态的工单，报错
  if (selectedOrderIds.value.some((id) => {
    const order = userOrders.value.find(o => o.orderID === id)
    return !order || order.status !== '待分发'
  })) {
    ElMessage.warning('请选择待分发状态的工单')
    return
  }

  // 如果没有选择任何工单，则自动选中所有待分发工单
  if (selectedOrderIds.value.length === 0) {
    selectedOrderIds.value = userOrders.value
      .filter(order => order.status === '待分发')
      .map(order => order.orderID)
  }

  batchDistributeDialogVisible.value = true
}

// 新增：获取批量操作时的模型ID（取第一个选中工单的模型ID）
function getBatchModelId(): string | undefined {
  if (selectedOrderIds.value.length === 0) {
    return undefined
  }

  const firstSelectedOrder = userOrders.value.find(order =>
    order.orderID === selectedOrderIds.value[0],
  )

  return firstSelectedOrder?.modelID
}

// 批量审批确认
const batchTargetDeliveryTime = ref<string>('')
async function confirmBatchApprove() {
  // 验证预计发送时间：如果有交付发送或版本迭代+交付发送类型的工单，必须填写预计发送时间
  const hasDeliveryOrders = userOrders.value.filter(o => selectedOrderIds.value.includes(o.orderID)).some(o => ['交付发送', '版本迭代+交付发送'].includes(o.type))
  if (hasDeliveryOrders && !batchTargetDeliveryTime.value) {
    ElMessage.error('请选择预计发送时间')
    return
  }
  
  let successCount = 0
  for (const orderID of selectedOrderIds.value) {
    const order = userOrders.value.find(o => o.orderID === orderID)
    if (order && order.status === '待审批') {
      order.referencePriority = batchLeaderPriority.value
      order.status = '待分发'
      order.distributorID = batchDistributorID.value
      order.approverID = currentUserId
      order.approveTime = new Date().toISOString().slice(0, 19).replace('T', ' ')
      // 批量设置预计发送时间（仅交付发送和版本迭代+交付发送）
      if (['交付发送', '版本迭代+交付发送'].includes(order.type)) {
        order.targetDeliveryTime = batchTargetDeliveryTime.value
      }
      const res = await orderApi.approveOrder({
        orderID: order.orderID,
        status: order.status,
        orderType: order.type,
        approverID: String(order.approverID ?? ''),
        approveTime: order.approveTime,
        referencePriority: order.referencePriority,
        targetDeliveryTime: order.targetDeliveryTime ?? '',
        distributorID: String(order.distributorID ?? ''),
      })
      if (res?.status === 1) {
        successCount++
      }
    }
  }
  batchApproveDialogVisible.value = false
  selectedOrderIds.value = []
  ElMessage.success(`已批量审批${successCount}个工单`)
  await fetchUserOrders()
}

// 批量分发确认
async function confirmBatchDistribute() {
  let successCount = 0
  for (const orderID of selectedOrderIds.value) {
    const order = userOrders.value.find(o => o.orderID === orderID)
    if (order && order.status === '待分发') {
      // 设置任务优先级和执行人ID
      order.taskPriority = batchTaskPriority.value
      order.status = '进行中'
      order.executorID = batchExecutorID.value
      order.distributorID = currentUserId
      order.distributeTime = new Date().toISOString().slice(0, 19).replace('T', ' ')
      // 调用单个分发接口
      const res = await orderApi.distributeOrder({
        orderID: order.orderID,
        status: order.status,
        distributorID: String(order.distributorID ?? ''),
        distributeTime: order.distributeTime,
        taskPriority: order.taskPriority,
        executorID: String(order.executorID ?? ''),
        orderType: order.type, // 工单类型
      })
      if (res?.status === 1) {
        successCount++
      }
    }
  }
  batchDistributeDialogVisible.value = false
  selectedOrderIds.value = []
  ElMessage.success(`已批量分发${successCount}个工单`)
  await fetchUserOrders()
}

// 处理工单选择框的勾选与取消勾选事件
function handleSelectOrder(orderID: string, checked: boolean) {
  const idx = selectedOrderIds.value.indexOf(orderID) // 查找当前工单ID在已选列表中的索引
  if (checked && idx === -1) {
    // 如果勾选且未在列表中，则添加到已选工单ID数组
    selectedOrderIds.value.push(orderID)
  }
  else if (!checked && idx !== -1) {
    // 如果取消勾选且已在列表中，则从已选工单ID数组中移除
    selectedOrderIds.value.splice(idx, 1)
  }
}

// 获取分发人列表 - 接收模型ID参数
const distributor = ref<Array<{ id: string, name: string }>>([]) // 分发人列表
async function fetchDistributorList(modelId?: string) {
  if (!modelId) {
    console.warn('未提供模型ID，无法获取分发人列表')
    distributor.value = []
    return
  }

  try {
    const res = await orderApi.fetchDistributorList(modelId)
    // 返回格式 [{ id: '工号', name: '姓名' }]
    distributor.value = res?.data?.list || []
  }
  catch (error) {
    console.error('获取分发人列表失败:', error)
    distributor.value = []
  }
}

// 获取执行人列表 - 接收模型ID参数
const executor = ref<Array<{ id: string, name: string }>>([]) // 执行人列表
async function fetchExecutorList(modelId?: string) {
  if (!modelId) {
    console.warn('未提供模型ID，无法获取执行人列表')
    executor.value = []
    return
  }

  try {
    const res = await orderApi.fetchExecutorList(modelId)
    // 返回格式 [{ id: '工号', name: '姓名' }]
    executor.value = res?.data?.list || []
  }
  catch (error) {
    console.error('获取执行人列表失败:', error)
    executor.value = []
  }
}
// 授权ID列表和外壳号获取
const customerAuthIds = ref<Array<{
  authId: string
  authId_shellNumber: string
  authorizationId: string // 授权id主键
  remainingDays: string
  remainingDaysColor: string
  deviceType: string
  authNote: string
}>>([]) // 客户授权ID列表

// 根据目标客户获取授权ID列表
async function fetchCustomerAuthIds(order?: OrderItem) {
  try {
    const targetCustomer = order?.targetCustomer
    if (!targetCustomer) {
      console.warn('未找到目标客户信息，无法获取授权ID列表')
      customerAuthIds.value = []
      return
    }

    // 调用后端接口，传入目标客户参数
    const res = await orderApi.fetchCustomerAuthIds(targetCustomer)

    if (res?.data?.list) {
      // 展开每个authId下的shellNumberList
      customerAuthIds.value = res.data.list.flatMap((auth: any) =>
        (auth.shellNumberList || []).map((shell: any) => ({
          authId: auth.authId,
          authId_shellNumber: shell.shellNumber,
          remainingDays: calculateRemainingDays(shell.endTime),
          remainingDaysColor: getRemainingDaysColor(shell.endTime),
          deviceType: shell.deviceType,
          authNote: shell.description || '',
          authorizationId: shell.authorizationId || '',
        })),
      )
    }
    else {
      customerAuthIds.value = []
    }
  }
  catch (error) {
    console.error('获取授权ID列表失败:', error)
    customerAuthIds.value = []
    ElMessage.error('获取授权ID列表失败，请稍后重试')
  }
}

async function downloadFile(fileUrl: string, fileName: string) {
  try {
    // 从fileUrl中提取ticketId
    const urlParts = fileUrl.split('/')
    const ticketIdIndex = urlParts.findIndex(part => part === 'ticket')

    if (ticketIdIndex === -1 || ticketIdIndex + 1 >= urlParts.length) {
      throw new Error('无法从文件URL中提取工单ID')
    }

    const ticketId = Number.parseInt(urlParts[ticketIdIndex + 1])

    if (Number.isNaN(ticketId)) {
      throw new TypeError('工单ID格式不正确')
    }

    // 调用后端API下载文件
    const response = await orderApi.downloadTicketFile(ticketId, encodeURIComponent(fileName))

    // 从响应数据中提取文件信息
    const { content, contentType, size } = response.data

    // 将Base64内容转换为二进制数据
    const binaryString = window.atob(content)
    const bytes = new Uint8Array(binaryString.length)
    for (let i = 0; i < binaryString.length; i++) {
      bytes[i] = binaryString.charCodeAt(i)
    }

    // 创建blob对象
    const blob = new Blob([bytes], {
      type: contentType || 'application/octet-stream',
    })

    // 验证文件大小（可选）
    if (blob.size !== size) {
      console.warn(`文件大小不匹配: 期望 ${size} bytes, 实际 ${blob.size} bytes`)
    }

    // 创建下载URL
    const url = window.URL.createObjectURL(blob)

    // 创建下载链接
    const link = document.createElement('a')
    link.href = url
    link.download = fileName
    document.body.appendChild(link) // 确保添加到DOM
    link.click()
    document.body.removeChild(link) // 移除

    // 清理URL
    window.URL.revokeObjectURL(url)

    ElMessage.success('文件下载成功')
  }
  catch (error) {
    ElMessage.error('文件下载失败')
    console.error('下载文件失败:', error)
  }
}
// -------------流转功能-----------------
// 获取流转负责人列表 - 接收模型ID参数
const transferExecutor = ref<Array<{ id: string, name: string }>>([]) // 流转负责人列表
async function fetchTransferExecutorList(modelId?: string) {
  if (!modelId) {
    console.warn('未提供模型ID，无法获取流转负责人列表')
    transferExecutor.value = []
    return
  }

  try {
    const res = await orderApi.fetchTransferExecutorList(modelId)
    // 返回格式 [{ id: '工号', name: '姓名' }]
    transferExecutor.value = res?.data?.list || []
  }
  catch (error) {
    console.error('获取流转负责人列表失败:', error)
    transferExecutor.value = []
  }
}

// 提交流转事件处理函数
async function handleTransferOrder(order: OrderItem) {
  if (!order.transferReasonEdit || !order.transferExecutorIDEdit) {
    ElMessage.error('请填写完整流转信息')
    return
  }
  order.transferTimeEdit = new Date().toISOString().slice(0, 19).replace('T', ' ')

  // 工单处于待封装状态，调用封装流转接口
  if (order.statusTodo === '待封装') {
    const res = await orderApi.transferOrder_package({
      orderID: order.orderID, // 工单号
      executorID: String(currentUserId), // 当前用户ID
      transferExecutorID: String(order.transferExecutorIDEdit), // 流转负责人ID
      transferReason: order.transferReasonEdit, // 流转原因
      transferTime: order.transferTimeEdit, // 流转时间
    })

    // 显示后端返回的 message
    if (res?.data?.message) {
      ElMessage.success(res.data.message)
    }
    // 清空流转相关的输入框缓存
    order.transferReasonEdit = ''
    order.transferExecutorIDEdit = ''
    order.transferTimeEdit = ''
  }
  // 工单处于待加密状态，调用加密流转接口
  else if (order.statusTodo === '待加密') {
    const res = await orderApi.transferOrder_encrypted({
      orderID: order.orderID, // 工单号
      executorID: String(currentUserId), // 当前用户ID
      transferExecutorID: String(order.transferExecutorIDEdit), // 流转负责人ID
      transferReason: order.transferReasonEdit, // 流转原因
      transferTime: order.transferTimeEdit, // 流转时间
      finishModelVersion: order.finishModelVersion ?? '', // 更新版本
    })

    // 显示后端返回的 message
    if (res?.data?.message) {
      ElMessage.success(res.data.message)
    }
    // 清空流转相关的输入框缓存
    order.transferReasonEdit = ''
    order.transferExecutorIDEdit = ''
    order.transferTimeEdit = ''
  }
  // 工单处于待发送状态，调用发送流转接口
  else if (order.statusTodo === '待发送') {
    const res = await orderApi.transferOrder_send({
      orderID: order.orderID, // 工单号
      executorID: String(currentUserId), // 当前用户ID
      transferExecutorID: String(order.transferExecutorIDEdit), // 流转负责人ID
      transferReason: order.transferReasonEdit, // 流转原因
      transferTime: order.transferTimeEdit, // 流转时间
    })

    // 显示后端返回的 message
    if (res?.data?.message) {
      ElMessage.success(res.data.message)
    }
    // 清空流转相关的输入框缓存
    order.transferReasonEdit = ''
    order.transferExecutorIDEdit = ''
    order.transferTimeEdit = ''
  }
  // 其他工单填写完成工单时流转
  else {
    const res = await orderApi.transferOrder({
      orderID: order.orderID, // 工单号
      orderType: order.type, // 工单类型
      transferType: '完成工单流转', // 流转类型
      transferReason: order.transferReasonEdit, // 流转原因
      transferTime: order.transferTimeEdit, // 流转时间
      transferExecutorID: String(order.transferExecutorIDEdit), // 流转负责人ID
      executorID: String(currentUserId), // 当前用户ID
    })
    // 显示后端返回的 message
    if (res?.data?.message) {
      ElMessage.success(res.data.message)
    }
    // 清空流转相关的输入框缓存
    order.transferReasonEdit = ''
    order.transferExecutorIDEdit = ''
    order.transferTimeEdit = ''
  }

  // 刷新工单列表
  fetchUserOrders()
}

// ---------------授权ID详情------------------
// 授权ID选择弹窗相关状态
const authSelectDialogVisible = ref(false)
const currentOrder = ref<OrderItem | null>(null)
const authDetailsForSelection = ref<Array<{
  authId: string
  remainingDays: string
  remainingDaysColor: string
  deviceType: string
  authNote: string
}>>([])

// 处理授权ID下拉框点击事件
async function handleAuthIdSelectClick(order: OrderItem) {
  currentOrder.value = order

  try {
    // 获取目标客户的授权ID列表
    await fetchCustomerAuthIds(order)

    // 将获取到的授权ID列表设置到选择弹窗中
    authDetailsForSelection.value = customerAuthIds.value

    if (authDetailsForSelection.value.length > 0) {
      authSelectDialogVisible.value = true
    }
    else {
      ElMessage.warning('该客户暂无可用的授权ID')
    }
  }
  catch (error) {
    console.error('获取授权ID列表失败:', error)
    ElMessage.error('获取授权ID列表失败，请稍后重试')
  }
}

// 分组并合并授权ID
function groupAuthShells(list: Array<{
  authId: string
  authId_shellNumber: string
  authorizationId: string
  remainingDays: string
  remainingDaysColor: string
  deviceType: string
  authNote: string
}>) {
  interface Shell {
    shellNumber: string
    authorizationId: string // 新增
    remainingDays: string
    remainingDaysColor: string
    deviceType: string
    authNote: string
  }
  const map = new Map<string, { authId: string, shells: Shell[] }>()
  list.forEach((item) => {
    if (!map.has(item.authId)) {
      map.set(item.authId, {
        authId: item.authId,
        shells: [],
      })
    }
    map.get(item.authId)!.shells.push({
      shellNumber: item.authId_shellNumber,
      remainingDays: item.remainingDays,
      remainingDaysColor: item.remainingDaysColor,
      deviceType: item.deviceType,
      authNote: item.authNote,
      authorizationId: item.authorizationId, // 新增
    })
  })
  return Array.from(map.values())
}
// 选择授权ID和外壳号
function selectAuthIdShell(authId: string) {
  if (currentOrder.value) {
    // 找到当前分组下所有外壳号
    const shells = groupAuthShells(customerAuthIds.value).find(g => g.authId === authId)?.shells || []
    // 保存授权ID和所有外壳号到工单
    currentOrder.value.finishAuthId = authId
    currentOrder.value.finishShellNo = shells.map(s => s.shellNumber)
    currentOrder.value.authorizationId_list = shells.map(s => s.authorizationId)
  }
  authSelectDialogVisible.value = false
  currentOrder.value = null
  authDetailsForSelection.value = []
}

// 关闭授权ID选择弹窗
function handleCloseAuthSelect() {
  authSelectDialogVisible.value = false
  currentOrder.value = null
  authDetailsForSelection.value = []
}

// 计算剩余天数
function calculateRemainingDays(endDate: string): string {
  const today = new Date()
  const end = new Date(endDate)

  today.setHours(0, 0, 0, 0)
  end.setHours(0, 0, 0, 0)

  const diffTime = end.getTime() - today.getTime()
  const diffDays = Math.ceil(diffTime / (1000 * 60 * 60 * 24))

  if (diffDays > 0) {
    return `${diffDays}天`
  }
  else if (diffDays === 0) {
    return '今天到期'
  }
  else {
    return `已过期${Math.abs(diffDays)}天`
  }
}

// 获取剩余天数颜色
function getRemainingDaysColor(endDate: string): string {
  const today = new Date()
  const end = new Date(endDate)

  today.setHours(0, 0, 0, 0)
  end.setHours(0, 0, 0, 0)

  const diffTime = end.getTime() - today.getTime()
  const diffDays = Math.ceil(diffTime / (1000 * 60 * 60 * 24))

  if (diffDays > 5) {
    return 'text-green-600' // 有效：绿色
  }
  else if (diffDays >= 0 && diffDays <= 5) {
    return 'text-orange-600' // 临期：橙色
  }
  else {
    return 'text-gray-500' // 过期：灰色
  }
}

// 新增：解析完成模型版本的各部分
function parseCompleteModelVersion(completeModelVersion: string) {
  if (!completeModelVersion) {
    return { first: '', second: '', third: '' }
  }

  // 移除开头的V（如果有）
  const versionWithoutV = completeModelVersion.replace(/^V/, '')
  const parts = versionWithoutV.split('.')

  if (parts.length >= 3) {
    return {
      first: parts[0] || '', // 第一位数字：3
      second: parts[1] || '', // 第二位数字：2
      third: parts[2] || '', // 第三位数字：1
    }
  }

  return { first: '', second: '', third: '' }
}

// 新增：处理升级后模型版本的输入
function handleFinishVersionInput(order: OrderItem, field: 'number' | 'letter', value: string) {
  if (field === 'number') {
    order.finishModelVersionNumber = value.replace(/\D/g, '')
  }
  else if (field === 'letter') {
    order.finishModelVersionLetter = value.replace(/[^A-Z]/gi, '').toUpperCase()
  }
  const base = parseCompleteModelVersion(order.completeModelVersion || '')
  if (base.first && base.second && base.third) {
    let version = `${base.first}.${base.second}.${base.third}`
    if (order.finishModelVersionNumber) {
      version += `.${order.finishModelVersionNumber}${order.finishModelVersionLetter || ''}`
    }
    order.finishModelVersion = version
    console.warn('fin', order.finishModelVersion)
  }
}

// 页面加载时请求工单数据
onMounted(() => {
  fetchUserOrders()
})
</script>

<template>
  <div>
    <!-- 顶部标题栏 -->
    <div class="mb-6 border border-gray-200 rounded-lg bg-white p-6 shadow-sm">
      <div class="flex items-center justify-between">
        <div class="flex items-center">
          <h1 class="text-2xl text-gray-800 font-bold">
            工单待办
          </h1>
        </div>
      </div>

      <!-- 文字提示行 -->
      <div class="mt-3 text-sm text-gray-600">
        本页面显示当前用户的待办工单
      </div>
    </div>
    <el-skeleton :loading="loading" animated>
      <template #default>
        <!-- 一键审批/一键分发操作区 -->
        <FaPageMain>
          <template #title>
            <div class="w-full flex justify-center">
              <span class="text-2xl text-black font-bold">功能区</span>
            </div>
          </template>
          <div class="mb-4 w-full flex justify-center gap-6">
            <div class="mb-4 w-full flex justify-center gap-6">
              <el-tooltip content="请选择待审批工单" placement="top" :disabled="true">
                <el-button type="primary" @click="openBatchApproveDialog">
                  一键审批
                </el-button>
              </el-tooltip>
              <el-tooltip content="请选择待分发工单" placement="top" :disabled="true">
                <el-button type="success" @click="openBatchDistributeDialog">
                  一键分发
                </el-button>
              </el-tooltip>
            </div>
          </div>
        </FaPageMain>
        <div class="space-y-4">
          <!-- 每个工单一个内容块 -->
          <FaPageMain v-for="order in userOrders" :key="order.orderID" :title="`工单#${order.orderID}`"
            :collaspe="!expandedMap[order.orderID]" height="auto" class="w-full">
            <template #title>
              <div class="w-full flex items-center justify-between">
                <div>
                  <span class="block flex items-center text-lg text-black font-extrabold tracking-wide">
                    <!-- 工单前面加选择框 -->
                    <el-checkbox :model-value="selectedOrderIds.includes(order.orderID)"
                      style="margin-right: 12px; margin-left: 8px;"
                      @change="checked => handleSelectOrder(order.orderID, Boolean(checked))" />
                    <!-- 工单类型在前，字号加大加粗 -->
                    <span class="mr-4 text-xl font-black flex items-end" :class="{
                      'text-blue-700': order.type === '问题复现',
                      'text-green-700': order.type === '版本迭代',
                      'text-yellow-700': order.type === '交付发送',
                      'text-purple-700': order.type === '版本迭代+交付发送',
                      'text-pink-700': order.type === '功能开发',
                      'text-gray-700': order.type === '其他',
                    }">
                      {{ order.type }}
                      <!-- 工单号作为右下角小字体 -->
                      <span class="ml-2 text-sm text-gray-500">
                        工单#{{ order.orderID }}
                      </span>
                      <!-- CAE图标 -->
                      <!-- 接口变更图标 -->

                      <!-- <span v-if="order.targetPlatform === 'CAE'" class="ml-4 flex items-end">
                        <img src="@/assets/icons/Cae3D.svg" class="inline-block w-7 h-7" title="CAE" alt="CAE" />
                      </span>

                      <span v-if="order.apiChanged" :class="order.targetPlatform === 'CAE' ? 'ml-2' : 'ml-4'"
                        class="flex items-end">
                        <img src="@/assets/icons/接口.svg" class="inline-block w-5 h-6" title="接口变更" alt="接口变更" />
                      </span> -->
                      <div class="flex items-center gap-2 text-sm font-medium">
                        <span v-if="order.targetPlatform === 'CAE'" class=" ml-2 text-green-600">
                          CAE
                        </span>

                        <span v-if="order.apiChanged" class="ml-2 text-red-600">
                          接口变更
                        </span>

                        <span :class="order.taskPriority === '紧急' ? 'ml-2 text-red-600' : 'ml-2 text-green-600'">
                          {{ order.taskPriority }}
                        </span>

                        <span v-if="order.hasSensitiveInfo != '否'" class="ml-2 text-red-600">
                          敏感信息包含
                        </span>

                        <span v-if="order.isCAEChecked ==='是'" class="ml-2 text-green-600">
                          CAE-IPT平台验证通过
                        </span>

                        <!-- 这里可以继续加 -->

                      </div>



                    </span>
                  </span>
                  <div class="mt-2 flex flex-wrap items-center gap-6 text-sm">
                    <FaIcon name="expand" class="mr-0 cursor-pointer text-xl"
                      @click="expandOrder(order.orderID, !expandedMap[order.orderID])" />
                    <!-- 版本迭代+交付发送类工单 -->
                    <template v-if="order.type === '版本迭代+交付发送'">
                      <span>
                        <i class="i-mdi-cube mr-1 text-blue-400" />
                        <span class="text-gray-600">模型：</span>
                        <span class="text-black font-bold">{{ order.modelID }}</span>
                      </span>
                      <span>
                        <span class="text-gray-600">发送版本：</span>
                        <span class="text-black font-bold">{{ order.finishModelVersion }}</span>
                      </span>
                      <span>
                        <span class="text-gray-600">Matlab版本：</span>
                        <span class="text-black font-bold">{{ order.completeModelVersion || 'NA' }}</span>
                      </span>
                      <span>
                        <span class="text-gray-600">目标客户：</span>
                        <span class="text-black font-bold">{{ order.targetCustomer }}</span>
                      </span>
                      <span>
                        <span class="text-gray-600">预计发送时间：</span>
                        <span class="text-black font-bold">{{ order.targetDeliveryTime || 'NA' }}</span>
                      </span>
                      <span>
                        <span class="text-gray-600">当前状态：</span>
                        <span class="ml-2 rounded-full px-3 py-1 font-bold" :class="{
                          'bg-yellow-100 text-yellow-700': order.status === '草稿',
                          'bg-orange-100 text-orange-700': order.status === '待审批',
                          'bg-purple-100 text-purple-700': order.status === '待分发',
                          'bg-blue-100 text-blue-700': order.status === '进行中',
                          'bg-green-100 text-green-700': order.status === '已完成',
                          'bg-red-100 text-red-700': order.status === '已退回',
                        }">
                          {{ order.status }}
                        </span>
                      </span>
                    </template>
                    <!-- 交付发送类工单 -->
                    <template v-else-if="order.type === '交付发送'">
                      <span>
                        <i class="i-mdi-cube mr-1 text-blue-400" />
                        <span class="text-gray-600">模型：</span>
                        <span class="text-black font-bold">{{ order.modelID }}</span>
                      </span>
                      <span>
                        <span class="text-gray-600">发送版本：</span>
                        <span class="text-black font-bold">{{ order.modelVersionID }}</span>
                      </span>
                      <span>
                        <span class="text-gray-600">目标客户：</span>
                        <span class="text-black font-bold">{{ order.targetCustomer }}</span>
                      </span>
                      <span>
                        <span class="text-gray-600">预计发送时间：</span>
                        <span class="text-black font-bold">{{ order.targetDeliveryTime || 'NA' }}</span>
                      </span>
                      <span>
                        <span class="text-gray-600">任务优先级：</span>
                        <span class="text-black font-bold">{{ order.taskPriority }}</span>
                      </span>
                      <span>
                        <span class="text-gray-600">当前状态：</span>
                        <span class="ml-2 rounded-full px-3 py-1 font-bold" :class="{
                          'bg-yellow-100 text-yellow-700': order.status === '草稿',
                          'bg-orange-100 text-orange-700': order.status === '待审批',
                          'bg-purple-100 text-purple-700': order.status === '待分发',
                          'bg-blue-100 text-blue-700': order.status === '进行中',
                          'bg-green-100 text-green-700': order.status === '已完成',
                          'bg-red-100 text-red-700': order.status === '已退回',
                        }">
                          {{ order.status }}
                        </span>
                      </span>
                    </template>
                    <!-- 版本迭代类工单 -->
                    <template v-else-if="order.type === '版本迭代'">
                      <span>
                        <i class="i-mdi-cube mr-1 text-blue-400" />
                        <span class="text-gray-600">模型：</span>
                        <span class="text-black font-bold">{{ order.modelID }}</span>
                      </span>
                      <span>
                        <span class="text-gray-600">基准版本：</span>
                        <span class="text-black font-bold">{{ order.modelVersionID }}</span>
                      </span>
                      <span>
                        <span class="text-gray-600">Matlab版本：</span>
                        <span class="text-black font-bold">{{ order.completeModelVersion || 'NA' }}</span>
                      </span>
                      <span>
                        <span class="text-gray-600">目标平台：</span>
                        <span class="text-black font-bold">{{ order.targetPlatform || 'NA' }}</span>
                      </span>
                      <span>
                        <span class="text-gray-600">任务优先级：</span>
                        <span class="text-black font-bold">{{ order.taskPriority }}</span>
                      </span>
                      <span>
                        <span class="text-gray-600">当前状态：</span>
                        <span class="ml-2 rounded-full px-3 py-1 font-bold" :class="{
                          'bg-yellow-100 text-yellow-700': order.status === '草稿',
                          'bg-orange-100 text-orange-700': order.status === '待审批',
                          'bg-purple-100 text-purple-700': order.status === '待分发',
                          'bg-blue-100 text-blue-700': order.status === '进行中',
                          'bg-green-100 text-green-700': order.status === '已完成',
                          'bg-red-100 text-red-700': order.status === '已退回',
                        }">
                          {{ order.status }}
                        </span>
                      </span>
                    </template>
                    <!-- 其他类型工单 -->
                    <template v-else>
                      <span>
                        <i class="i-mdi-cube mr-1 text-blue-400" />
                        <span class="text-gray-600">模型：</span>
                        <span class="text-black font-bold">{{ order.modelID }}</span>
                      </span>
                      <span>
                        <span class="text-gray-600">基准版本：</span>
                        <span class="text-black font-bold">{{ order.modelVersionID }}</span>
                      </span>
                      <span v-if="order.type == '功能开发'">
                        <span class="text-gray-600">目标平台：</span>
                        <span class="text-black font-bold">{{ order.targetPlatform || 'NA' }}</span>
                      </span>
                      <span>
                        <span class="text-gray-600">当前状态：</span>
                        <span class="ml-2 rounded-full px-3 py-1 font-bold" :class="{
                          'bg-yellow-100 text-yellow-700': order.status === '草稿',
                          'bg-orange-100 text-orange-700': order.status === '待审批',
                          'bg-purple-100 text-purple-700': order.status === '待分发',
                          'bg-blue-100 text-blue-700': order.status === '进行中',
                          'bg-green-100 text-green-700': order.status === '已完成',
                          'bg-red-100 text-red-700': order.status === '已退回',
                        }">
                          {{ order.status }}
                        </span>
                      </span>
                    </template>
                  </div>
                </div>
              </div>
            </template>
            <!-- Tab切换区域 -->
            <div class="mt-6">
              <!-- Tab导航栏 -->
              <div
                class="relative rounded-t-lg p-1 border-l border-r border-t border-gray-200 before:absolute before:bottom-0 before:left-[-8px] before:w-2 before:h-2 before:bg-gray-50 before:rounded-br-lg before:border-r before:border-b before:border-gray-200 after:absolute after:bottom-0 after:right-[-8px] after:w-2 after:h-2 after:bg-gray-50 after:rounded-bl-lg after:border-l after:border-b after:border-gray-200">
                <nav class="flex space-x-1 w-full">
                  <button v-for="tab in getOrderTabs(order)" :key="tab.key" @click="switchTab(order.orderID, tab.key)"
                    class="relative whitespace-nowrap py-3 px-4 font-medium text-sm flex flex-col items-center gap-2 transition-all duration-300 ease-in-out transform hover:scale-102 w-full tab-button"
                    :class="{
                      'bg-white text-blue-700 border-t border-l border-r border-blue-200 rounded-t-md tab-active': getActiveTab(order.orderID) === tab.key,
                      'text-gray-600 hover:text-gray-800 hover:bg-white/50 rounded-md': getActiveTab(order.orderID) !== tab.key
                    }">
                    <!-- 激活状态的顶部指示条 -->
                    <div v-if="getActiveTab(order.orderID) === tab.key"
                      class="absolute top-0 left-1/2 transform -translate-x-1/2 w-8 h-1 bg-blue-500 rounded-full"></div>

                    <!-- 底部延伸线条 -->
                    <div v-if="getActiveTab(order.orderID) === tab.key"
                      class="absolute bottom-0 left-0 right-0 h-0.5 bg-blue-500 extending-line"></div>

                    <div class="flex items-center gap-3">
                      <!-- 标签名称 -->
                      <span class="font-semibold">{{ tab.label }}</span>

                      <!-- 状态指示器 -->
                      <div class="relative">
                        <span class="inline-block w-3 h-3 rounded-full border-2 border-white shadow-sm" :class="{
                          'bg-green-500': tab.status === 'completed',
                          'bg-orange-500': tab.status === 'in_progress',
                          'bg-red-500': tab.status === 'rejected',
                          'bg-gray-400': !tab.status || tab.status === 'pending'
                        }" :title="tab.status === 'completed' ? '已完成' :
                          tab.status === 'in_progress' ? '进行中' :
                            tab.status === 'rejected' ? '已退回' : '未开始'"></span>
                        <!-- 脉冲动画（进行中状态） -->
                        <span v-if="tab.status === 'in_progress'"
                          class="absolute top-1/6 left-0 w-3 h-3 bg-orange-500 rounded-full animate-ping opacity-75"></span>
                      </div>
                    </div>

                    <!-- 负责人信息 -->
                    <div v-if="tab.responsible" class="flex items-center gap-1">
                      <i class="i-mdi-account text-xs opacity-70"></i>
                      <span class="text-xs font-medium opacity-80">{{ tab.responsible }}</span>
                    </div>
                  </button>
                </nav>
              </div>

              <!-- Tab内容区域 -->
              <div class="mt-0">
                <!-- 完成工单 Tab面板 -->
                <div
                  v-if="getActiveTab(order.orderID) === 'complete' && ['进行中', '已完成'].includes(order.status) && ['问题复现', '功能开发', '其他'].includes(order.type)"
                  class="bg-white border-l border-r border-b border-gray-200 rounded-b-lg shadow-sm p-4">
                  <div class="mb-4 flex items-center justify-between">
                    <div class="flex items-center">
                      <span class="text-lg text-blue-900 font-bold">完成工单</span>
                      <span v-if="order.status === '进行中'" class="ml-2 inline-block align-middle"
                        style="width: 12px;height: 12px;background: #f59e42;border-radius: 50%;" title="进行中" />
                      <span v-else-if="order.status === '已完成'" class="ml-2 inline-block align-middle"
                        style="width: 12px;height: 12px;background: #22c55e;border-radius: 50%;" title="已完成" />
                    </div>
                    <div class="flex items-center gap-4 text-sm text-gray-700 font-bold">
                      <span>负责人：{{ order.executorID }}</span>
                      <span>完成时间：{{ order.finishTime }}</span>
                    </div>
                  </div>
                <div
                  class="grid grid-cols-2 items-start gap-x-8 gap-y-4 rounded bg-gray-50 px-6 py-4 border-0 border-gray-300 relative "
                  :class="order.status === '进行中' ? 'bg-gray-50' : 'bg-gray-100 opacity-70'">
                  <!-- 问题复现类工单 -->
                  <template v-if="order.type === '问题复现'">


                    <!-- 创建工单备注： -->
                    <div class="col-span-2 w-full flex flex-col gap-2">
                      <span class="w-32 text-black font-semibold">创建工单备注：</span>
                      <textarea v-model="order.createRemark"
                        class="w-full resize-none border-0 border-b border-gray-300 bg-transparent py-2 text-sm text-black focus:outline-none focus:border-blue-500"
                        rows="2" readonly />
                    </div>
                    <div class="col-span-1 w-full flex flex-col gap-2">
                      <span class="text-black font-semibold">复现现象：</span>
                      <textarea v-model="order.finishPhenomenon"
                        class="w-full resize-none border-0 border-b border-gray-300 bg-transparent py-2 text-sm text-black focus:outline-none focus:border-blue-500"
                        rows="2" :readonly="order.status === '已完成'" />
                    </div>
                    <div class="col-span-1 w-full flex flex-col gap-2">
                      <span class="text-black font-semibold">备注：</span>
                      <textarea v-model="order.finishRemark"
                        class="w-full resize-none border-0 border-b border-gray-300 bg-transparent py-2 text-sm text-black focus:outline-none focus:border-blue-500"
                        rows="2" :readonly="order.status === '已完成'" />
                    </div>
                  </template>
                  <!-- 功能开发类工单 -->
                  <template v-else-if="order.type === '功能开发'">
                    <div class="col-span-2 w-full flex items-center gap-2">
                      <span class="w-40 text-black font-semibold">
                        <span class="mr-1 text-red-500">*</span>
                        完成后版本：
                      </span>
                      <div class="flex flex-1 flex-col gap-2">
                        <!-- 验证码样式的版本输入 -->
                        <div v-if="order.status !== '已完成'" class="version-input-container">
                          <!-- 显示创建时的完成版本作为基础 -->
                          <template v-if="order.completeModelVersion">
                            <template
                              v-if="parseCompleteModelVersion(order.completeModelVersion || '').first && parseCompleteModelVersion(order.completeModelVersion || '').second && parseCompleteModelVersion(order.completeModelVersion || '').third">
                              <!-- 第一个数字 -->
                              <div class="version-part readonly">
                                {{ parseCompleteModelVersion(order.completeModelVersion || '').first }}
                              </div>
                              <div class="version-part static">
                                .
                              </div>
                              <!-- 第二个数字 -->
                              <div class="version-part readonly">
                                {{ parseCompleteModelVersion(order.completeModelVersion || '').second }}
                              </div>
                              <div class="version-part static">
                                .
                              </div>
                              <!-- 第三个数字 -->
                              <div class="version-part readonly">
                                {{ parseCompleteModelVersion(order.completeModelVersion || '').third }}
                              </div>
                              <div class="version-part static">
                                .
                              </div>
                            </template>
                            <!-- 数字输入框 -->
                            <el-input v-model="order.finishModelVersionNumber" placeholder="0" maxlength="5"
                              class="version-input"
                              @input="value => handleFinishVersionInput(order, 'number', value)" />
                            <!-- 字母输入框 -->
                            <el-input v-model="order.finishModelVersionLetter" placeholder="A" maxlength="4"
                              class="version-input"
                              @input="value => handleFinishVersionInput(order, 'letter', value)" />
                          </template>

                          <!-- 如果没有completeModelVersion，显示传统输入框 -->
                          <template v-else>
                            <input v-model="order.finishModelVersion"
                              class="border-0 border-b border-gray-300 bg-transparent py-2 text-sm text-black focus:outline-none focus:border-blue-500"
                              :placeholder="`当前版本: ${order.modelVersionID}`">
                          </template>
                        </div>

                        <!-- 已完成状态：只读显示 -->
                        <div v-else class="version-input-container">
                          <template v-if="order.finishModelVersion">
                            <template v-for="(char, index) in order.finishModelVersion.split('')" :key="index">
                              <!-- 点号显示为静态样式 -->
                              <div v-if="char === '.'" class="version-part static">
                                .
                              </div>
                              <!-- 数字和字母显示为只读样式 -->
                              <div v-else class="version-part readonly">
                                {{ char }}
                              </div>
                            </template>
                          </template>
                          <!-- 如果没有完成版本，显示提示 -->
                          <template v-else>
                            <div class="text-gray-500 italic">
                              未填写升级后版本
                            </div>
                          </template>
                        </div>

                        <!-- 完整版本预览 -->
                        <div v-if="order.finishModelVersion && order.status !== '已完成'" class="text-xs text-gray-600">
                          <span class="font-semibold">完整版本:</span>
                          <span class="ml-2 rounded bg-blue-50 px-2 py-1 text-blue-700 font-bold font-mono">
                            {{ order.finishModelVersion }}
                          </span>
                        </div>

                        <!-- 说明文字 -->
                        <div v-if="order.status !== '已完成'" class="text-xs text-gray-500">
                          <span v-if="order.completeModelVersion">
                            基于创建工单时选择的版本 <strong>{{ order.completeModelVersion }}</strong>，
                            请输入第4位数字和第5位字母
                          </span>
                          <!-- 展开后显示已使用版本或无已使用版本提示 -->
                          <div v-if="expandedMap[order.orderID]" class="mt-1 text-blue-600">
                            <template v-if="order.usedModelVersions && order.usedModelVersions.length">
                              已使用版本：{{ order.usedModelVersions.join('，') }}
                            </template>
                            <template v-else>
                              暂无已使用版本
                            </template>
                          </div>
                          <span v-else>
                            请输入升级后的模型版本号
                          </span>
                        </div>
                      </div>
                    </div>

                    <!-- 创建工单备注： -->
                    <div class="col-span-2 w-full flex items-center gap-2">
                      <span class="w-32 text-black font-semibold">创建工单备注：</span>
                      <textarea v-model="order.createRemark"
                        class="flex-1 resize-none border-0 border-b border-gray-300 bg-transparent py-2 text-sm text-black focus:outline-none focus:border-blue-500"
                        rows="2" readonly />
                    </div>
                  </template>

                  <!-- 其他类工单 -->
                  <template v-else-if="order.type === '其他'">


                    <!-- 创建工单备注： -->
                    <div class="col-span-2 w-full flex items-center gap-2">
                      <span class="w-32 text-black font-semibold">创建工单备注：</span>
                      <textarea v-model="order.createRemark"
                        class="flex-1 resize-none border-0 border-b border-gray-300 bg-transparent py-2 text-sm text-black focus:outline-none focus:border-blue-500"
                        rows="2" readonly />
                    </div>
                    <div class="col-span-2 w-full flex flex-col gap-2">
                      <span class="text-black font-semibold">备注：</span>
                      <textarea v-model="order.finishRemarkOther"
                        class="w-full resize-none border-0 border-b border-gray-300 bg-transparent py-2 text-sm text-black focus:outline-none focus:border-blue-500"
                        rows="2" :readonly="order.status === '已完成'" />
                    </div>
                  </template>
                  <!-- 修改完成工单内容块，按钮与流转信息分三行（流转按钮单独一行） -->
                  <div class="col-span-2 mt-4 flex flex-col items-center gap-4">
                    <!-- 第一行：提交完成工单按钮 -->
                    <el-button type="primary" size="large"
                      :disabled="order.status === '已完成' || !isFinishOrderFilled(order)"
                      @click="handleFinishOrderClick(order)">
                      提交完成工单
                    </el-button>

                    <!-- 第二行：分割线 -->
                    <hr class="my-4 w-full border-t-2 border-gray-300">
                    <!-- 第三行：流转内容区（直接绑定到 order） -->
                    <div class="w-full flex items-center gap-2">
                      <span class="w-32 text-black font-semibold whitespace-nowrap">流转负责人：</span>
                      <el-select v-model="order.transferExecutorIDEdit" placeholder="请选择流转负责人ID" filterable clearable
                        size="small" class="w-[160px]" :disabled="order.status === '已完成'"
                        @visible-change="val => val && fetchTransferExecutorList(order.modelID)">
                        <el-option v-for="item in transferExecutor" :key="item.id" :label="`${item.name} (${item.id})`"
                          :value="item.id" />
                      </el-select>
                      <span class="ml-4 w-32 text-black font-semibold whitespace-nowrap">
                        <span class="mr-1 text-red-500">*</span>
                        工作记录：</span>
                      <textarea v-model="order.transferReasonEdit"
                        class="flex-1 resize-none border-0 border-b border-gray-300 bg-transparent py-2 text-sm text-black focus:outline-none focus:border-blue-500"
                        rows="2" :readonly="order.status === '已完成'" style="min-width: 320px;" placeholder="请输入工作记录" />
                    </div>
                    <div class="w-full flex items-center justify-center">
                      <el-button type="primary" size="large"
                        :disabled="order.status === '已完成' || !order.transferExecutorIDEdit || !order.transferReasonEdit"
                        @click="handleTransferOrder(order)">
                        提交流转
                      </el-button>
                    </div>
                  </div>
                </div>

                </div>
              </div>

              <!-- 发送工单 Tab面板 -->
              <div
                v-if="getActiveTab(order.orderID) === 'send' && ['进行中', '已完成'].includes(order.status) && order.type === '交付发送'"
                class="bg-white border-l border-r border-b border-gray-200 rounded-b-lg shadow-sm p-4">
                <div class="mb-4 flex items-center justify-between">
                  <div class="flex items-center">
                    <span class="text-lg text-blue-900 font-bold">发送工单</span>
                    <span v-if="order.status === '进行中'" class="ml-2 inline-block align-middle"
                      style="width: 12px;height: 12px;background: #f59e42;border-radius: 50%;" title="进行中" />
                    <span v-else-if="order.status === '已完成'" class="ml-2 inline-block align-middle"
                      style="width: 12px;height: 12px;background: #22c55e;border-radius: 50%;" title="已完成" />
                  </div>
                  <div class="flex items-center gap-4 text-sm text-gray-700 font-bold">
                    <span>负责人：{{ order.currentExecutor }}</span>
                    <span>完成时间：{{ order.sendAt }}</span>
                  </div>
                </div>
                <div
                  class="grid grid-cols-2 items-start gap-x-8 gap-y-4 rounded-lg border border-gray-200 relative px-6 py-4"
                  :class="order.status === '进行中' ? 'bg-green-50' : 'bg-gray-50'" style="position: relative;">
                  <!-- 授权ID（只读） -->
                  <div class="flex items-center gap-2 flex-1">
                    <span class="w-32 text-black font-semibold">授权ID：</span>
                    <input :value="order.finishAuthId"
                      class="w-full border-0 border-b border-gray-300 bg-transparent py-2 text-sm text-black focus:outline-none focus:border-blue-500"
                      readonly>
                  </div>
                  <!-- 发送版本（只读） -->
                  <div class="flex items-center gap-2 flex-1">
                    <span class="w-32 text-black font-semibold">发送版本：</span>
                    <input :value="order.modelVersionID"
                      class="w-full border-0 border-b border-gray-300 bg-transparent py-2 text-sm text-black focus:outline-none focus:border-blue-500"
                      readonly>
                  </div>
                  <!-- 目标客户（只读） -->
                  <div class="flex items-center gap-2 flex-1">
                    <span class="w-32 text-black font-semibold">目标客户：</span>
                    <input :value="order.targetCustomer"
                      class="w-full border-0 border-b border-gray-300 bg-transparent py-2 text-sm text-black focus:outline-none focus:border-blue-500"
                      readonly>
                  </div>
                  <!-- 发送备注 -->
                  <div class="col-span-2 w-full flex flex-col gap-2">
                    <span class="w-32 text-black font-semibold">发送备注：</span>
                    <textarea v-model="order.sendRemark"
                      class="w-full resize-none border-0 border-b border-gray-300 bg-transparent py-2 text-sm text-black focus:outline-none focus:border-blue-500"
                      rows="2" :readonly="order.statusTodo !== '待发送'" />
                  </div>
                  <!-- 只有待发送时显示提交按钮和流转区 -->
                  <div v-if="order.statusTodo === '待发送'" class="col-span-2 mt-4 flex flex-col items-center gap-4">
                    <el-button type="primary" size="large"
                      :disabled="order.status === '已完成' || !isFinishOrderFilled(order)"
                      @click="handleFinishOrderClick(order)">
                      提交发送工单
                    </el-button>
                    <hr class="my-4 w-full border-t-2 border-gray-300">
                    <div class="w-full flex items-center gap-2">
                      <span class="w-32 text-black font-semibold whitespace-nowrap">流转负责人：</span>
                      <el-select v-model="order.transferExecutorIDEdit" placeholder="请选择流转负责人ID" filterable clearable
                        size="small" style="width: 140px;" :disabled="order.status === '已完成'"
                        @visible-change="val => val && fetchTransferExecutorList(order.modelID)">
                        <el-option v-for="item in transferExecutor" :key="item.id" :label="`${item.name} (${item.id})`"
                          :value="item.id" />
                      </el-select>
                      <span class="ml-4 w-32 text-black font-semibold whitespace-nowrap">
                        <span class="mr-1 text-red-500">*</span>
                        工作记录：</span>
                      <textarea v-model="order.transferReasonEdit"
                        class="flex-1 resize-none border-0 border-b border-gray-300 bg-transparent py-2 text-sm text-black focus:outline-none focus:border-blue-500"
                        rows="2" :readonly="order.status === '已完成'" style="min-width: 320px;" placeholder="请输入工作记录" />
                    </div>
                    <div class="w-full flex items-center justify-center">
                      <el-button type="primary" size="large"
                        :disabled="order.status === '已完成' || !order.transferExecutorIDEdit || !order.transferReasonEdit"
                        @click="handleTransferOrder(order)">
                        提交流转
                      </el-button>
                    </div>
                  </div>
                  <!-- 已完成时只读展示已填写信息，无编辑和提交按钮 -->
                  <div v-else-if="order.statusTodo === '已完成'" class="col-span-2 mt-4 flex flex-col gap-4">
                    <div class="flex flex-wrap gap-4">
                      <span class="w-32 text-black font-semibold">发送备注：</span>
                      <span class="font-bold text-gray-700">{{ order.sendRemark || '未填写' }}</span>
                    </div>
                  </div>
                </div>
              </div>

              <!-- 版本迭代+交付发送类工单-发送 Tab面板 -->
              <div
                v-if="getActiveTab(order.orderID) === 'send' && ['进行中', '已完成'].includes(order.status) && order.type === '版本迭代+交付发送'"
                class="bg-white border-l border-r border-b border-gray-200 rounded-b-lg shadow-sm p-4">
                <div class="mb-4 flex items-center justify-between">
                  <div class="flex items-center">
                    <span class="text-lg text-blue-900 font-bold">发送工单</span>
                    <span v-if="order.status === '进行中'" class="ml-2 inline-block align-middle"
                      style="width: 12px;height: 12px;background: #f59e42;border-radius: 50%;" title="进行中" />
                    <span v-else-if="order.status === '已完成'" class="ml-2 inline-block align-middle"
                      style="width: 12px;height: 12px;background: #22c55e;border-radius: 50%;" title="已完成" />
                  </div>
                  <div class="flex items-center gap-4 text-sm text-gray-700 font-bold">
                    <span>负责人：{{ order.currentExecutor }}</span>
                    <span>完成时间：{{ order.finishTime }}</span>
                  </div>
                </div>
                <div
                  class="grid grid-cols-2 items-start gap-x-8 gap-y-4 rounded-lg border border-gray-200 relative px-6 py-4"
                  :class="order.status === '进行中' ? 'bg-green-50' : 'bg-gray-50'" style="position: relative;">
                  <!-- 授权ID（只读） -->
                  <div class="flex items-center gap-2 flex-1">
                    <span class="w-32 text-black font-semibold">授权ID：</span>
                    <input :value="order.finishAuthId"
                      class="w-full border-0 border-b border-gray-300 bg-transparent py-2 text-sm text-black focus:outline-none focus:border-blue-500"
                      readonly>
                  </div>
                  <!-- 发送版本（只读） -->
                  <div class="flex items-center gap-2 flex-1">
                    <span class="w-32 text-black font-semibold">发送版本：</span>
                    <input :value="order.modelVersionID"
                      class="w-full border-0 border-b border-gray-300 bg-transparent py-2 text-sm text-black focus:outline-none focus:border-blue-500"
                      readonly>
                  </div>
                  <!-- 目标客户（只读） -->
                  <div class="flex items-center gap-2 flex-1">
                    <span class="w-32 text-black font-semibold">目标客户：</span>
                    <input :value="order.targetCustomer"
                      class="w-full border-0 border-b border-gray-300 bg-transparent py-2 text-sm text-black focus:outline-none focus:border-blue-500"
                      readonly>
                  </div>
                  <!-- 发送备注 -->
                  <div class="col-span-2 w-full flex flex-col gap-2">
                    <span class="w-27 text-black font-semibold">创建工单备注：</span>
                    <textarea v-model="order.createRemark"
                      class="w-full resize-none border-0 border-b border-gray-300 bg-transparent py-2 text-sm text-black focus:outline-none focus:border-blue-500"
                      rows="2" readonly />
                  </div>
                  <!-- 发送备注 -->
                  <div class="col-span-2 w-full flex flex-col gap-2">
                    <span class="w-27 text-black font-semibold">发送备注：</span>
                    <textarea v-model="order.sendRemark"
                      class="w-full resize-none border-0 border-b border-gray-300 bg-transparent py-2 text-sm text-black focus:outline-none focus:border-blue-500"
                      rows="2" :readonly="order.statusTodo !== '待发送'" />
                  </div>
                  <!-- 只有待发送时显示提交按钮和流转区 -->
                  <div v-if="order.statusTodo === '待发送'" class="col-span-2 mt-4 flex flex-col items-center gap-4">
                    <el-button type="primary" size="large"
                      :disabled="order.status === '已完成' || !isFinishOrderFilled(order)"
                      @click="handleFinishOrderClick(order)">
                      提交发送工单
                    </el-button>
                    <hr class="my-4 w-full border-t-2 border-gray-300">
                    <div class="w-full flex items-center gap-2">
                      <span class="w-32 text-black font-semibold whitespace-nowrap">流转负责人：</span>
                      <el-select v-model="order.transferExecutorIDEdit" placeholder="请选择流转负责人ID" filterable clearable
                        size="small" style="width: 140px;" :disabled="order.status === '已完成'"
                        @visible-change="val => val && fetchTransferExecutorList(order.modelID)">
                        <el-option v-for="item in transferExecutor" :key="item.id" :label="`${item.name} (${item.id})`"
                          :value="item.id" />
                      </el-select>
                      <span class="ml-4 w-32 text-black font-semibold whitespace-nowrap">
                        <span class="mr-1 text-red-500">*</span>
                        工作记录：</span>
                      <textarea v-model="order.transferReasonEdit"
                        class="flex-1 resize-none border-0 border-b border-gray-300 bg-transparent py-2 text-sm text-black focus:outline-none focus:border-blue-500"
                        rows="2" :readonly="order.status === '已完成'" style="min-width: 320px;" placeholder="请输入工作记录" />
                    </div>
                    <div class="w-full flex items-center justify-center">
                      <el-button type="primary" size="large"
                        :disabled="order.status === '已完成' || !order.transferExecutorIDEdit || !order.transferReasonEdit"
                        @click="handleTransferOrder(order)">
                        提交流转
                      </el-button>
                    </div>
                  </div>
                  <!-- 已完成时只读展示已填写信息，无编辑和提交按钮 -->
                  <div v-else-if="order.statusTodo === '已完成'" class="col-span-2 mt-4 flex flex-col gap-4">
                    <div class="flex flex-wrap gap-4">
                      <span class="w-32 text-black font-semibold">发送备注：</span>
                      <span class="font-bold text-gray-700">{{ order.sendRemark || '未填写' }}</span>
                    </div>
                  </div>
                </div>
              </div>

              <!-- 发送环节流转 Tab面板 -->
              <div
                v-if="getActiveTab(order.orderID) === 'transfer' && order.transfers_Delivery && order.transfers_Delivery.length"
                class="bg-white border-l border-r border-b border-gray-200 rounded-b-lg shadow-sm p-4">
                <template v-for="(transfer, idx) in order.transfers_Delivery" :key="idx">
                  <div class="mb-4 flex items-center justify-between">
                    <div class="flex items-center">
                      <span class="text-lg text-green-900 font-bold">
                        发送环节流转
                        {{ order.transfers_Delivery.length > 1 ? `（第${order.transfers_Delivery.length - idx}次）` : '' }}
                      </span>
                      <span class="ml-2 inline-block align-middle"
                        style="width: 12px;height: 12px;background: #22c55e ;border-radius: 50%;" title="发送流转" />
                    </div>
                    <div class="flex items-center gap-4 text-sm text-gray-700 font-bold">
                      <span>流转发起人：{{ transfer.transferCreatorID }}</span>
                      <span>流转执行人：{{ transfer.transferExecutorID }}</span>
                      <span>流转时间：{{ transfer.transferTime }}</span>
                    </div>
                  </div>
                  <div
                    class="grid grid-cols-2 items-start gap-x-8 gap-y-4 rounded-lg border border-gray-200 relative bg-green-50 px-6 py-4"
                    style="position: relative;">
                    <div class="col-span-2 w-full flex flex-col gap-2">
                      <span class="w-32 text-black font-semibold">工作记录：</span>
                      <textarea
                        class="w-full resize-none border-0 border-b border-gray-300 bg-transparent py-2 text-sm text-black focus:outline-none focus:border-blue-500"
                        :value="transfer.transferReason" rows="2" readonly />
                    </div>
                  </div>
                </template>
              </div>

              <!-- 交付发送类工单-加密 Tab面板 -->
              <div
                v-if="getActiveTab(order.orderID) === 'encrypt' && ['进行中', '已完成'].includes(order.status) && order.type === '交付发送' && ['待加密', '待发送'].includes(order.statusTodo ?? '')"
                class="bg-white border-l border-r border-b border-gray-200 rounded-b-lg shadow-sm p-4">
                <div class="mb-4 flex items-center justify-between">
                  <div class="w-full flex items-center justify-between">
                    <div class="flex items-center">
                      <span class="text-lg text-blue-900 font-bold">加密工单</span>
                      <span v-if="order.status === '进行中' && order.statusTodo === '待加密'"
                        class="ml-2 inline-block align-middle"
                        style="width: 12px;height: 12px;background: #f59e42;border-radius: 50%;" title="进行中" />
                      <span v-else-if="order.status === '进行中' && order.statusTodo === '待发送'"
                        class="ml-2 inline-block align-middle"
                        style="width: 12px;height: 12px;background: #22c55e;border-radius: 50%;" title="已完成" />
                    </div>
                    <div class="flex items-center gap-4 text-sm text-gray-700 font-bold">
                      <span>
                        负责人：{{ order.statusTodo === '待加密' ? order.currentExecutor : order.encryptedExecutorID }}
                      </span>
                      <span>完成时间：{{ order.encryptedAt }}</span>
                    </div>
                  </div>
                </div>
                <div
                  class="grid grid-cols-2 items-start gap-x-8 gap-y-4 rounded-lg border border-gray-200 relative bg-green-50 px-6 py-4"
                  style="position: relative;">
                  <!-- 编辑区，仅待加密时可填写 -->
                  <template v-if="order.statusTodo === '待加密'">


                    <!-- 创建工单备注： -->
                    <div class="col-span-2 w-full flex flex-col gap-2">
                      <span class="w-32 text-black font-semibold">创建工单备注：</span>
                      <textarea v-model="order.createRemark"
                        class="w-full resize-none border-0 border-b border-gray-300 bg-transparent py-2 text-sm text-black focus:outline-none focus:border-blue-500"
                        rows="2" readonly />
                    </div>
                    <div class="col-span-2 w-full flex items-center gap-2">
                      <span class="w-32 text-black font-semibold">
                        <span class="mr-1 text-red-500">*</span>
                        是否加密：</span>
                      <el-select v-model="order.isEncrypted" placeholder="请选择" class="min-w-[100px] w-full"
                        :disabled="order.statusTodo !== '待加密'">
                        <el-option label="是" value="是" />
                        <el-option label="否" value="否" />
                      </el-select>
                    </div>
                    <template v-if="order.isEncrypted === '是'">
                      <div class="col-span-2 w-full flex items-center gap-2">
                        <span class="w-32 text-black font-semibold">
                          <span class="mr-1 text-red-500">*</span>
                          授权ID：
                        </span>
                        <el-button type="primary" size="default" :disabled="order.statusTodo !== '待加密'"
                          @click="handleAuthIdSelectClick(order)">
                          选择
                        </el-button>
                        <span v-if="order.finishAuthId" class="ml-3 text-blue-700 font-bold">
                          {{ order.finishAuthId }}
                        </span>
                        <span v-else class="ml-3 text-gray-400">未选择</span>
                      </div>
                    </template>
                    <div class="col-span-2 w-full flex flex-col gap-2">
                      <span class="w-32 text-black font-semibold">加密备注：</span>
                      <textarea v-model="order.encryptedRemark"
                        class="w-full resize-none border-0 border-b border-gray-300 bg-transparent py-2 text-sm text-black focus:outline-none focus:border-blue-500"
                        rows="2" :readonly="order.statusTodo !== '待加密'" />
                    </div>
                    <div class="col-span-2 w-full flex items-center gap-2">
                      <span class="mr-1 text-red-500">*</span>
                      <span class="w-32 text-black font-semibold">发送人：</span>
                      <el-select v-model="order.sendExecutorID" placeholder="请选择发送人" filterable clearable size="default"
                        class="min-w-[120px] w-full" :disabled="order.statusTodo !== '待加密'"
                        @visible-change="val => val && fetchExecutorList(order.modelID)">
                        <el-option v-for="item in executor" :key="item.id" :label="`${item.name} (${item.id})`"
                          :value="item.id" />
                      </el-select>
                    </div>
                    <!-- 提交和流转区 -->
                    <div class="col-span-2 mt-4 flex flex-col items-center gap-4">
                      <el-button type="primary" size="large"
                        :disabled="order.status === '已完成' || !isFinishOrderFilled(order)"
                        @click="handleFinishOrderClick(order)">
                        提交加密工单
                      </el-button>
                      <hr class="my-4 w-full border-t-2 border-gray-300">
                      <div class="w-full flex items-center gap-2">
                        <span class="w-32 text-black font-semibold whitespace-nowrap">流转负责人：</span>
                        <el-select v-model="order.transferExecutorIDEdit" placeholder="请选择流转负责人ID" filterable clearable
                          size="small" class="w-[160px]" :disabled="order.status === '已完成'"
                          @visible-change="val => val && fetchTransferExecutorList(order.modelID)">
                          <el-option v-for="item in transferExecutor" :key="item.id"
                            :label="`${item.name} (${item.id})`" :value="item.id" />
                        </el-select>
                        <span class="ml-4 w-32 text-black font-semibold whitespace-nowrap">
                          <span class="mr-1 text-red-500">*</span>
                          工作记录：</span>
                        <textarea v-model="order.transferReasonEdit"
                          class="flex-1 resize-none border-0 border-b border-gray-300 bg-transparent py-2 text-sm text-black focus:outline-none focus:border-blue-500"
                          rows="2" :readonly="order.status === '已完成'" placeholder="请输入工作记录" />
                      </div>
                      <div class="w-full flex items-center justify-center">
                        <el-button type="primary" size="large"
                          :disabled="order.status === '已完成' || !order.transferExecutorIDEdit || !order.transferReasonEdit"
                          @click="handleTransferOrder(order)">
                          提交流转
                        </el-button>
                      </div>
                    </div>
                  </template>
                  <!-- 只读区，仅待发送时显示，美化展示 -->
                  <template v-else-if="order.statusTodo === '待发送'">
                    <div class="space-y-3 w-full">
                      <div class="flex items-center gap-2">
                        <span class="w-32 text-black font-semibold">是否加密：</span>
                        <span class="rounded bg-blue-50 px-2 py-1 font-bold text-blue-700">
                          {{ order.isEncrypted || '未填写' }}
                        </span>
                      </div>
                      <template v-if="order.isEncrypted === '是'">
                        <div class="flex items-center gap-2">
                          <span class="w-32 text-black font-semibold">授权ID：</span>
                          <span class="rounded bg-blue-50 px-2 py-1 font-bold text-blue-700">
                            {{ order.finishAuthId || '未填写' }}
                          </span>
                        </div>
                      </template>
                      <div class="flex items-center gap-2">
                        <span class="w-32 text-black font-semibold">加密备注：</span>
                        <span class="rounded bg-gray-50 px-2 py-1 font-bold text-gray-700">
                          {{ order.encryptedRemark || '未填写' }}
                        </span>
                      </div>
                    </div>
                  </template>
                </div>
              </div>

              <!-- 版本迭代+交付发送类工单-加密 Tab面板 -->
              <div
                v-if="getActiveTab(order.orderID) === 'encrypt' && ['进行中'].includes(order.status) && order.type === '版本迭代+交付发送' && ['待加密', '待发送'].includes(order.statusTodo ?? '')"
                class="bg-white border-l border-r border-b border-gray-200 rounded-b-lg shadow-sm p-4">
                <div class="mb-4 flex items-center justify-between">
                  <div class="w-full flex items-center justify-between">
                    <div class="flex items-center">
                      <span class="text-lg text-blue-900 font-bold">加密工单</span>
                      <span v-if="order.status === '进行中'" class="ml-2 inline-block align-middle"
                        style="width: 12px;height: 12px;background: #f59e42;border-radius: 50%;" title="进行中" />
                      <span v-else-if="order.status === '已完成'" class="ml-2 inline-block align-middle"
                        style="width: 12px;height: 12px;background: #22c55e;border-radius: 50%;" title="已完成" />
                    </div>
                    <div class="flex items-center gap-4 text-sm text-gray-700 font-bold">
                      <span>
                        负责人：{{ order.statusTodo === '待加密' ? order.currentExecutor : order.encryptedExecutorID }}
                      </span>
                      <span>完成时间：{{ order.encryptedAt }}</span>
                    </div>
                  </div>
                </div>
                <div
                  class="grid grid-cols-2 items-start gap-x-8 gap-y-4 rounded-lg border border-gray-200 relative bg-green-50 px-6 py-4"
                  style="position: relative;">
                  <!-- 编辑区，仅待加密时可填写 -->
                  <template v-if="order.statusTodo === '待加密'">


                    <!-- 创建工单备注： -->
                    <div class="col-span-2 w-full flex items-center gap-2">
                      <span class="w-32 text-black font-semibold">创建工单备注：</span>
                      <textarea v-model="order.createRemark"
                        class="flex-1 resize-none border-0 border-b border-gray-300 bg-transparent py-2 text-sm text-black focus:outline-none focus:border-blue-500"
                        rows="2" readonly />
                    </div>
                    <div class="col-span-2 w-full flex items-center gap-2">
                      <span class="w-32 text-black font-semibold">
                        <span class="mr-1 text-red-500">*</span>
                        是否加密：</span>
                      <el-select v-model="order.isEncrypted" placeholder="请选择" class="flex-1"
                        :disabled="order.statusTodo !== '待加密'">
                        <el-option label="是" value="是" />
                        <el-option label="否" value="否" />
                      </el-select>
                    </div>
                    <template v-if="order.isEncrypted === '是'">
                      <div class="col-span-2 w-full flex items-center gap-2">
                        <span class="w-32 text-black font-semibold">
                          <span class="mr-1 text-red-500">*</span>
                          授权ID：
                        </span>
                        <el-button type="primary" size="default" :disabled="order.statusTodo !== '待加密'"
                          @click="handleAuthIdSelectClick(order)">
                          选择
                        </el-button>
                        <span v-if="order.finishAuthId" class="ml-3 text-blue-700 font-bold">
                          {{ order.finishAuthId }}
                        </span>
                        <span v-else class="ml-3 text-gray-400">未选择</span>
                      </div>
                    </template>
                    <div class="col-span-2 w-full flex flex-col gap-2">
                      <span class="w-32 text-black font-semibold">加密备注：</span>
                      <textarea v-model="order.encryptedRemark"
                        class="w-full resize-none border-0 border-b border-gray-300 bg-transparent py-2 text-sm text-black focus:outline-none focus:border-blue-500"
                        rows="2" :readonly="order.statusTodo !== '待加密'" />
                    </div>
                    <div class="col-span-2 w-full flex items-center gap-2">
                      <span class="mr-1 text-red-500">*</span>
                      <span class="w-32 text-black font-semibold">发送人：</span>
                      <el-select v-model="order.sendExecutorID" placeholder="请选择发送人" filterable clearable size="default"
                        style="width: 160px;" :disabled="order.statusTodo !== '待加密'"
                        @visible-change="val => val && fetchExecutorList(order.modelID)">
                        <el-option v-for="item in executor" :key="item.id" :label="`${item.name} (${item.id})`"
                          :value="item.id" />
                      </el-select>
                    </div>
                    <!-- 提交和流转区 -->
                    <div class="col-span-2 mt-4 flex flex-col items-center gap-4">
                      <el-button type="primary" size="large"
                        :disabled="order.status === '已完成' || !isFinishOrderFilled(order)"
                        @click="handleFinishOrderClick(order)">
                        提交加密工单
                      </el-button>
                      <hr class="my-4 w-full border-t-2 border-gray-300">
                      <div class="w-full flex items-center gap-2">
                        <span class="w-32 text-black font-semibold whitespace-nowrap">流转负责人：</span>
                        <el-select v-model="order.transferExecutorIDEdit" placeholder="请选择流转负责人ID" filterable clearable
                          size="small" style="width: 140px;" :disabled="order.status === '已完成'"
                          @visible-change="val => val && fetchTransferExecutorList(order.modelID)">
                          <el-option v-for="item in transferExecutor" :key="item.id"
                            :label="`${item.name} (${item.id})`" :value="item.id" />
                        </el-select>
                        <span class="ml-4 w-32 text-black font-semibold whitespace-nowrap">
                          <span class="mr-1 text-red-500">*</span>
                          工作记录：</span>
                        <textarea v-model="order.transferReasonEdit"
                          class="flex-1 resize-none border-0 border-b border-gray-300 bg-transparent py-2 text-sm text-black focus:outline-none focus:border-blue-500"
                          rows="2" :readonly="order.status === '已完成'" placeholder="请输入工作记录" />
                      </div>
                      <div class="w-full flex items-center justify-center">
                        <el-button type="primary" size="large"
                          :disabled="order.status === '已完成' || !order.transferExecutorIDEdit || !order.transferReasonEdit"
                          @click="handleTransferOrder(order)">
                          提交流转
                        </el-button>
                      </div>
                    </div>
                  </template>
                  <!-- 只读区，仅待发送时显示，美化展示 -->
                  <template v-else-if="order.statusTodo === '待发送'">
                    <div class="space-y-3 w-full">
                      <div class="flex items-center gap-2">
                        <span class="w-32 text-black font-semibold">是否加密：</span>
                        <span class="rounded bg-blue-50 px-2 py-1 font-bold text-blue-700">
                          {{ order.isEncrypted || '未填写' }}
                        </span>
                      </div>
                      <template v-if="order.isEncrypted === '是'">
                        <div class="flex items-center gap-2">
                          <span class="w-32 text-black font-semibold">授权ID：</span>
                          <span class="rounded bg-blue-50 px-2 py-1 font-bold text-blue-700">
                            {{ order.finishAuthId || '未填写' }}
                          </span>
                        </div>
                      </template>
                      <div class="flex items-center gap-2">
                        <span class="w-32 text-black font-semibold">加密备注：</span>
                        <span class="rounded bg-gray-50 px-2 py-1 font-bold text-gray-700">
                          {{ order.encryptedRemark || '未填写' }}
                        </span>
                      </div>
                    </div>
                  </template>
                </div>
              </div>
            </div>
            <!-- </FaPageMain> -->

            <!-- 加密环节流转内容块 -->
            <template v-if="order.transfers_Encrypted && order.transfers_Encrypted.length && getActiveTab(order.orderID) === 'encrypt' ">
              <template v-for="(transfer, idx) in order.transfers_Encrypted" :key="idx">
                <FaPageMain title="" :collaspe="!expandedMap[order.orderID]" height="auto" class="w-full">
                  <template #title>
                    <div class="w-full flex items-center justify-between">
                      <div class="flex items-center">
                        <span class="text-lg text-purple-900 font-bold">
                          加密环节流转
                          {{ order.transfers_Encrypted.length > 1 ? `（第${order.transfers_Encrypted.length - idx}次）` : ''
                          }}
                        </span>
                        <span class="ml-2 inline-block align-middle"
                          style="width: 12px;height: 12px;background: #a855f7 ;border-radius: 50%;" title="加密流转" />
                      </div>
                      <div class="flex items-center gap-4 text-sm text-gray-700 font-bold">
                        <span>流转发起人：{{ transfer.transferCreatorID }}</span>
                        <span>流转执行人：{{ transfer.transferExecutorID }}</span>
                        <span>流转时间：{{ transfer.transferTime }}</span>
                      </div>
                    </div>
                  </template>
                  <div class="grid grid-cols-2 items-start gap-x-8 gap-y-4 rounded bg-purple-50 px-6 py-4">
                    <div class="col-span-2 w-full flex items-center gap-2">
                      <span class="w-32 text-black font-semibold">工作记录：</span>
                      <textarea
                        class="flex-1 resize-none border border-gray-200 rounded bg-purple-50 px-3 py-2 text-sm text-black"
                        :value="transfer.transferReason" rows="2" readonly />
                    </div>
                  </div>
                </FaPageMain>
              </template>
            </template>

            <!-- 版本迭代+交付发送类工单-封装 -->
            <div
              v-if="['进行中'].includes(order.status) && order.type === '版本迭代+交付发送' && ['待封装', '待加密', '待发送'].includes(order.statusTodo ?? '')  && getActiveTab(order.orderID) === 'package'"
              title="" :collaspe="!expandedMap[order.orderID]" height="auto" class="w-full border-l border-b border-r  rounded-b-lg border-gray-300 p-4">
              <div>
                <div class="w-full flex items-center justify-between">
                  <div class="flex items-center">
                    <span class="text-lg text-blue-900 font-bold">封装工单</span>
                    <span v-if="order.status === '进行中'" class="ml-2 inline-block align-middle"
                      style="width: 12px;height: 12px;background: #f59e42;border-radius: 50%;" title="进行中" />
                    <span v-else-if="order.status === '已完成'" class="ml-2 inline-block align-middle"
                      style="width: 12px;height: 12px;background: #22c55e;border-radius: 50%;" title="已完成" />
                  </div>
                  <div class="flex items-center gap-4 text-sm text-gray-700 font-bold">
                    <span>
                      负责人：{{ order.statusTodo === '待封装' ? order.currentExecutor : order.packageExecutorID }}
                    </span>
                    <span>完成时间：{{ order.packageAt }}</span>
                  </div>
                </div>
              </div>
              <div class="grid grid-cols-2 items-start gap-x-8 gap-y-4 rounded bg-gray-50 px-6 py-4 "
                :class="order.status === '进行中' ? 'bg-green-50' : 'bg-gray-100 opacity-70'">
                <!-- 第一行：升级后模型版本 -->
                <template v-if="order.statusTodo === '待封装'">


                  <!-- 创建工单备注： -->
                  <div class="col-span-2 w-full flex items-center gap-2">
                    <span class="w-32 text-black font-semibold">创建工单备注：</span>
                    <textarea v-model="order.createRemark"
                      class="flex-1 resize-none border-0 border-b border-gray-300 bg-transparent py-2 text-sm text-black focus:outline-none focus:border-blue-500"
                      rows="2" readonly />
                  </div>
                  <div class="col-span-1 w-full flex items-center gap-2">
                    <span class="w-40 text-black font-semibold">
                      <span class="mr-1 text-red-500">*</span>
                      升级后版本：</span>
                    <div class="flex flex-1 flex-col gap-2">
                      <!-- 编辑输入逻辑（仅待封装时可编辑） -->
                      <div v-if="order.statusTodo === '待封装'" class="version-input-container">
                        <template v-if="order.completeModelVersion">
                          <template
                            v-if="parseCompleteModelVersion(order.completeModelVersion || '').first && parseCompleteModelVersion(order.completeModelVersion || '').second && parseCompleteModelVersion(order.completeModelVersion || '').third">
                            <div class="version-part readonly">
                              {{ parseCompleteModelVersion(order.completeModelVersion || '').first }}
                            </div>
                            <div class="version-part static">.</div>
                            <div class="version-part readonly">
                              {{ parseCompleteModelVersion(order.completeModelVersion || '').second }}
                            </div>
                            <div class="version-part static">.</div>
                            <div class="version-part readonly">
                              {{ parseCompleteModelVersion(order.completeModelVersion || '').third }}
                            </div>
                            <div class="version-part static">.</div>
                          </template>
                          <el-input v-model="order.finishModelVersionNumber" placeholder="0" maxlength="5"
                            class="version-input" @input="value => handleFinishVersionInput(order, 'number', value)" />
                          <el-input v-model="order.finishModelVersionLetter" placeholder="A" maxlength="4"
                            class="version-input" @input="value => handleFinishVersionInput(order, 'letter', value)" />
                        </template>
                        <template v-else>
                          <input v-model="order.finishModelVersion"
                            class="border-0 border-b border-gray-300 bg-transparent py-2 text-sm text-black focus:outline-none focus:border-blue-500"
                            :placeholder="`当前版本: ${order.modelVersionID}`">
                        </template>
                      </div>
                      <!-- 只读显示（待加密、待发送、已完成） -->
                      <div v-else class="version-input-container">
                        <template v-if="order.finishModelVersion">
                          <template v-for="(char, index) in order.finishModelVersion.split('')" :key="index">
                            <div v-if="char === '.'" class="version-part static">.</div>
                            <div v-else class="version-part readonly">{{ char }}</div>
                          </template>
                        </template>
                        <template v-else>
                          <div class="text-gray-500 italic">未填写升级后版本</div>
                        </template>
                      </div>
                      <!-- 完整版本预览 -->
                      <div v-if="order.finishModelVersion && order.statusTodo === '待封装'" class="text-xs text-gray-600">
                        <span class="font-semibold">完整版本:</span>
                        <span class="ml-2 rounded bg-blue-50 px-2 py-1 text-blue-700 font-bold font-mono">
                          {{ order.finishModelVersion }}
                        </span>
                      </div>
                      <!-- 说明文字 -->
                      <div v-if="order.statusTodo === '待封装'" class="text-xs text-gray-500">
                        <span v-if="order.completeModelVersion">
                          基于创建工单时选择的版本 <strong>{{ order.completeModelVersion }}</strong>，
                          请输入第4位数字和第5位字母
                        </span>
                        <div v-if="expandedMap[order.orderID]" class="mt-1 text-blue-600">
                          <template v-if="order.usedModelVersions && order.usedModelVersions.length">
                            已使用版本：{{ order.usedModelVersions.join('，') }}
                          </template>
                          <template v-else>
                            暂无已使用版本
                          </template>
                        </div>
                        <span v-else>
                          请输入升级后的模型版本号
                        </span>
                        <div v-if="order.apiChanged" class="mt-2 flex items-center gap-2">
                          <span class="text-xs font-semibold"
                            :class="order.apiChanged === '是' ? 'text-red-600' : 'text-green-600'">
                            <i class="i-mdi-alert-circle-outline mr-1" />
                            接口与{{ order.modelVersionID || '基准版本' }}是否变化：
                            <span>{{ order.apiChanged }}</span>
                          </span>
                        </div>
                      </div>
                    </div>
                  </div>
                  <!-- 第三行：备注 -->
                  <div class="col-span-2 w-full flex items-center gap-2">
                    <span class="w-32 text-black font-semibold">封装备注：</span>
                    <textarea v-model="order.packageRemark"
                      class="flex-1 resize-none border-0 border-b border-gray-300 bg-transparent py-2 text-sm text-black focus:outline-none focus:border-blue-500"
                      rows="2" :readonly="order.statusTodo !== '待封装'" />
                  </div>
                  <!-- 选择下一步加密人 -->
                  <div class="col-span-2 w-full flex items-center gap-2">
                    <span class="mr-1 text-red-500">*</span>
                    <span class="w-32 text-black font-semibold">加密人：</span>
                    <el-select v-model="order.encryptedExecutorIDEdit" placeholder="请选择加密人" filterable clearable
                      size="default" class="min-w-[120px] w-full" :disabled="order.statusTodo !== '待封装'"
                      @visible-change="val => val && fetchExecutorList(order.modelID)">
                      <el-option v-for="item in executor" :key="item.id" :label="`${item.name} (${item.id})`"
                        :value="item.id" />
                    </el-select>
                  </div>

                  <!-- 只有待封装时显示提交按钮和流转区 -->
                  <div class="col-span-2 mt-4 flex flex-col items-center gap-4">
                    <el-button type="primary" size="large"
                      :disabled="order.status === '已完成' || !order.encryptedExecutorIDEdit || !order.completeModelVersion || !order.finishModelVersionLetter || !order.finishModelVersionNumber"
                      @click="handleFinishOrderClick(order)">
                      提交封装工单
                    </el-button>
                    <hr class="my-4 w-full border-t-2 border-gray-300">
                    <div class="w-full flex items-center gap-2">
                      <span class="w-32 text-black font-semibold whitespace-nowrap">流转负责人：</span>
                      <el-select v-model="order.transferExecutorIDEdit" placeholder="请选择流转负责人ID" filterable clearable
                        size="small" style="width: 140px;" :disabled="order.status === '已完成'"
                        @visible-change="val => val && fetchTransferExecutorList(order.modelID)">
                        <el-option v-for="item in transferExecutor" :key="item.id" :label="`${item.name} (${item.id})`"
                          :value="item.id" />
                      </el-select>
                      <span class="ml-4 w-32 text-black font-semibold whitespace-nowrap">
                        <span class="mr-1 text-red-500">*</span>
                        工作记录：</span>
                      <textarea v-model="order.transferReasonEdit"
                        class="flex-1 resize-none border-0 border-b border-gray-300 bg-transparent py-2 text-sm text-black focus:outline-none focus:border-blue-500"
                        rows="2" :readonly="order.status === '已完成'" style="min-width: 320px;" placeholder="请输入工作记录" />
                    </div>
                    <div class="w-full flex items-center justify-center">
                      <el-button type="primary" size="large"
                        :disabled="order.status === '已完成' || !order.transferExecutorIDEdit || !order.transferReasonEdit"
                        @click="handleTransferOrder(order)">
                        提交流转
                      </el-button>
                    </div>
                  </div>
                </template>
                <!-- 只读展示：流程非待封装时，美化展示 -->
                <template v-else-if="['待加密', '待发送'].includes(order.statusTodo ?? '')">
                  <div class="space-y-3 w-full">
                    <div class="flex items-center gap-2">
                      <span class="w-32 text-black font-semibold">升级后版本：</span>
                      <span class="flex-1 rounded px-2 py-1 font-bold text-blue-700" style="background: #f0f6ff;">
                        {{ order.finishModelVersion || '未填写' }}
                      </span>
                    </div>
                    <div class="flex items-center gap-2">
                      <span class="w-32 text-black font-semibold">封装备注：</span>
                      <span class="flex-1 rounded px-2 py-1 font-bold text-gray-700" style="background: #f8fafc;">
                        {{ order.packageRemark || '未填写' }}
                      </span>
                    </div>
                  </div>
                </template>
              </div>
            </div>

            <!-- 版本迭代类工单-封装 -->
            <div v-if="['进行中'].includes(order.status) && order.type === '版本迭代'  && getActiveTab(order.orderID) === 'package'" title=""
              :collaspe="!expandedMap[order.orderID]" height="auto" class="w-full m-0 border-l border-b border-r  rounded-b-lg border-gray-300 p-4">
              <div>
                <div class="w-full flex items-center justify-between">
                  <div class="flex items-center">
                    <span class="text-lg text-blue-900 font-bold">封装工单</span>
                    <span v-if="order.status === '进行中'" class="ml-2 inline-block align-middle"
                      style="width: 12px;height: 12px;background: #f59e42;border-radius: 50%;" title="进行中" />
                    <span v-else-if="order.status === '已完成'" class="ml-2 inline-block align-middle"
                      style="width: 12px;height: 12px;background: #22c55e;border-radius: 50%;" title="已完成" />
                  </div>
                  <div class="flex items-center gap-4 text-sm text-gray-700 font-bold">
                    <span>负责人：{{ order.currentExecutor }}</span>
                    <span>完成时间：{{ order.finishTime }}</span>
                  </div>
                </div>
              </div>
              <div class="grid grid-cols-2 items-start gap-x-8 gap-y-4 rounded bg-gray-50 px-6 py-4 relative"
                :class="order.status === '进行中' ? 'bg-green-50' : 'bg-gray-100 opacity-70'">
                <!-- 版本迭代类工单 -->
                <div class="col-span-1 w-full flex items-center gap-2">
                  <span class="w-40 text-black font-semibold">
                    <span class="mr-1 text-red-500">*</span>
                    升级后版本：</span>
                  <div class="flex flex-1 flex-col gap-2">
                    <!-- 验证码样式的版本输入 -->
                    <div v-if="order.status !== '已完成'" class="version-input-container">
                      <!-- 显示创建时的完成版本作为基础 -->
                      <template v-if="order.completeModelVersion">
                        <template
                          v-if="parseCompleteModelVersion(order.completeModelVersion || '').first && parseCompleteModelVersion(order.completeModelVersion || '').second && parseCompleteModelVersion(order.completeModelVersion || '').third">
                          <!-- 第一个数字 -->
                          <div class="version-part readonly">
                            {{ parseCompleteModelVersion(order.completeModelVersion || '').first }}
                          </div>
                          <div class="version-part static">
                            .
                          </div>

                          <!-- 第二个数字 -->
                          <div class="version-part readonly">
                            {{ parseCompleteModelVersion(order.completeModelVersion || '').second }}
                          </div>
                          <div class="version-part static">
                            .
                          </div>

                          <!-- 第三个数字 -->
                          <div class="version-part readonly">
                            {{ parseCompleteModelVersion(order.completeModelVersion || '').third }}
                          </div>
                          <div class="version-part static">
                            .
                          </div>
                        </template>

                        <!-- 数字输入框 -->
                        <el-input v-model="order.finishModelVersionNumber" placeholder="0" maxlength="5"
                          class="version-input" @input="value => handleFinishVersionInput(order, 'number', value)" />
                        <!-- 字母输入框 -->
                        <el-input v-model="order.finishModelVersionLetter" placeholder="A" maxlength="4"
                          class="version-input" @input="value => handleFinishVersionInput(order, 'letter', value)" />
                      </template>

                      <!-- 如果没有completeModelVersion，显示传统输入框 -->
                      <template v-else>
                        <input v-model="order.finishModelVersion"
                          class="border-0 border-b border-gray-300 bg-transparent py-2 text-sm text-black focus:outline-none focus:border-blue-500"
                          :placeholder="`当前版本: ${order.modelVersionID}`">
                      </template>
                    </div>

                    <!-- 已完成状态：只读显示 -->
                    <div v-else class="version-input-container">
                      <template v-if="order.finishModelVersion">
                        <template v-for="char in order.finishModelVersion.split('')" :key="char">
                          <div class="version-part readonly">
                            {{ char }}
                          </div>
                        </template>
                      </template>
                    </div>

                    <!-- 完整版本预览 -->
                    <div v-if="order.finishModelVersion && order.status !== '已完成'" class="text-xs text-gray-600">
                      <span class="font-semibold">完整版本:</span>
                      <span class="ml-2 rounded bg-blue-50 px-2 py-1 text-blue-700 font-bold font-mono">
                        {{ order.finishModelVersion }}
                      </span>
                    </div>

                    <!-- 说明文字 -->
                    <div v-if="order.status !== '已完成'" class="text-xs text-gray-500">
                      <span v-if="order.completeModelVersion">
                        基于创建工单时选择的版本 <strong>{{ order.completeModelVersion }}</strong>，
                        请输入第4位数字和第5位字母
                      </span>
                      <!-- 展开后显示已使用版本或无已使用版本提示 -->
                      <div v-if="expandedMap[order.orderID]" class="mt-1 text-blue-600">
                        <template v-if="order.usedModelVersions && order.usedModelVersions.length">
                          已使用版本：{{ order.usedModelVersions.join('，') }}
                        </template>
                        <template v-else>
                          暂无已使用版本
                        </template>
                      </div>
                      <span v-else>
                        请输入升级后的模型版本号
                      </span>
                      <!-- 新增：接口变化提醒 -->
                      <div v-if="order.apiChanged" class="mt-2 flex items-center gap-2">
                        <span class="text-xs font-semibold"
                          :class="order.apiChanged === '是' ? 'text-red-600' : 'text-green-600'">
                          <i class="i-mdi-alert-circle-outline mr-1" />
                          接口与{{ order.modelVersionID || '基准版本' }}是否变化：
                          <span>{{ order.apiChanged }}</span>
                        </span>
                      </div>
                    </div>
                  </div>
                </div>
                <div class="col-span-2 w-full flex items-center gap-2">
                  <span class="w-32 text-black font-semibold">封装备注：</span>
                  <textarea v-model="order.packageRemark"
                    class="flex-1 resize-none border-0 border-b border-gray-300 bg-transparent py-2 text-sm text-black focus:outline-none focus:border-blue-500"
                    rows="2" :readonly="order.status === '已完成'" />
                </div>

                <!-- 修改完成工单内容块，按钮与流转信息分三行（流转按钮单独一行） -->
                <div class="col-span-2 mt-4 flex flex-col items-center gap-4">
                  <!-- 第一行：提交完成工单按钮 -->
                  <el-button type="primary" size="large"
                    :disabled="order.status === '已完成' || !isFinishOrderFilled(order)"
                    @click="handleFinishOrderClick(order)">
                    提交封装工单
                  </el-button>

                  <!-- 第二行：分割线 -->
                  <hr class="my-4 w-full border-t-2 border-gray-300">
                  <!-- 第三行：流转内容区（直接绑定到 order） -->
                  <div class="w-full flex items-center gap-2">
                    <span class="w-32 text-black font-semibold whitespace-nowrap">流转负责人：</span>
                    <el-select v-model="order.transferExecutorIDEdit" placeholder="请选择流转负责人ID" filterable clearable
                      size="small" style="width: 140px;" :disabled="order.status === '已完成'"
                      @visible-change="val => val && fetchTransferExecutorList(order.modelID)">
                      <el-option v-for="item in transferExecutor" :key="item.id" :label="`${item.name} (${item.id})`"
                        :value="item.id" />
                    </el-select>
                    <span class="ml-4 w-32 text-black font-semibold whitespace-nowrap">
                      <span class="mr-1 text-red-500">*</span>
                      工作记录：</span>
                    <textarea v-model="order.transferReasonEdit"
                      class="flex-1 resize-none border-0 border-b border-gray-300 bg-transparent py-2 text-sm text-black focus:outline-none focus:border-blue-500"
                      rows="2" :readonly="order.status === '已完成'" style="min-width: 320px;" placeholder="请输入工作记录" />
                  </div>
                  <div class="w-full flex items-center justify-center">
                    <el-button type="primary" size="large"
                      :disabled="order.status === '已完成' || !order.transferExecutorIDEdit || !order.transferReasonEdit"
                      @click="handleTransferOrder(order)">
                      提交流转
                    </el-button>
                  </div>
                </div>
              </div>
            </div>

            <!-- 封装环节流转内容块 -->
            <template v-if="order.transfers_Version && order.transfers_Version.length  && getActiveTab(order.orderID) === 'package'">
              <template v-for="(transfer, idx) in order.transfers_Version" :key="idx">
                <FaPageMain title="" :collaspe="!expandedMap[order.orderID]" height="auto" class="w-full">
                  <template #title>
                    <div class="w-full flex items-center justify-between">
                      <div class="flex items-center">
                        <span class="text-lg text-blue-900 font-bold">
                          封装环节流转
                          {{ order.transfers_Version.length > 1 ? `（第${order.transfers_Version.length - idx}次）` : '' }}
                        </span>
                        <span class="ml-2 inline-block align-middle"
                          style="width: 12px;height: 12px;background: #3b82f6 ;border-radius: 50%;" title="封装流转" />
                      </div>
                      <div class="flex items-center gap-4 text-sm text-gray-700 font-bold">
                        <span>流转发起人：{{ transfer.transferCreatorID }}</span>
                        <span>流转执行人：{{ transfer.transferExecutorID }}</span>
                        <span>流转时间：{{ transfer.transferTime }}</span>
                      </div>
                    </div>
                  </template>
                  <div class="grid grid-cols-2 items-start gap-x-8 gap-y-4 rounded bg-blue-50 px-6 py-4">
                    <div class="col-span-2 w-full flex items-center gap-2">
                      <span class="w-32 text-black font-semibold">工作记录：</span>
                      <textarea
                        class="flex-1 resize-none border-0 border-b border-gray-300 bg-blue-50 py-2 text-sm text-black focus:outline-none focus:border-blue-500"
                        :value="transfer.transferReason" rows="2" readonly />
                    </div>
                  </div>
                </FaPageMain>
              </template>
            </template>

            <!-- 多次流转内容块，循环显示每一次流转（紧跟在任务分发后面） -->
            <template v-if="order.status === '进行中'">
              <template v-for="(transfer, idx) in (order.transfers || [])" :key="idx">
                <FaPageMain v-if="getActiveTab(order.orderID) === 'complete'" title="" :collaspe="!expandedMap[order.orderID]" height="auto" class="w-full">
                  <template #title>
                    <div class="w-full flex items-center justify-between">
                      <div class="flex items-center">
                        <span class="text-lg text-blue-900 font-bold">
                          工单流转
                          {{ Array.isArray(order.transfers) && order.transfers.length > 1 ? `（第${order.transfers.length
                            - idx}次）` : '' }}
                        </span>
                        <span class="ml-2 inline-block align-middle"
                          style="width: 12px;height: 12px;background: #22c55e ;border-radius: 50%;" title="流转" />
                      </div>
                      <div class="flex items-center gap-4 text-sm text-gray-700 font-bold">
                        <span>流转发起人：{{ transfer.transferCreatorID }}</span>
                        <span>流转执行人：{{ transfer.transferExecutorID }}</span>
                        <span>流转时间：{{ transfer.transferTime }}</span>
                      </div>
                    </div>
                  </template>
                  <div class="grid grid-cols-2 items-start gap-x-8 gap-y-4 rounded bg-gray-50 px-6 py-4">
                    <div class="col-span-2 w-full flex items-center gap-2">
                      <span class="w-32 text-black font-semibold">工作记录：</span>
                      <textarea
                        class="flex-1 resize-none border-0 border-b border-gray-300 bg-gray-50 py-2 text-sm text-black focus:outline-none focus:border-blue-500"
                        :value="transfer.transferReason" rows="2" readonly />
                    </div>
                  </div>
                </FaPageMain>
              </template>
            </template>

            <!-- 任务分发 -->

            <div v-if="['待分发', '进行中'].includes(order.status)  && order.distributorID !== '' && getActiveTab(order.orderID) === 'distribute'" title="" :collaspe="!expandedMap[order.orderID]"
              height="auto" class="w-full border-l border-b border-r  rounded-b-lg border-gray-300 p-4">
              <div>
                <div class="w-full flex items-center justify-between pr-4">
                  <div class="flex items-center">
                    <span class="text-lg text-blue-900 font-bold">任务分发</span>
                    <!-- 只有"待分发"时橙点，"进行中"或"已完成"时绿点 -->
                    <span v-if="order.status === '待分发'" class="ml-2 inline-block align-middle"
                      style="width: 12px;height: 12px;background: #f59e42;border-radius: 50%;" title="进行中" />
                    <span v-else-if="['进行中', '已完成'].includes(order.status)" class="ml-2 inline-block align-middle"
                      style="width: 12px;height: 12px;background: #22c55e;border-radius: 50%;" title="已完成" />
                  </div>
                  <!-- 负责人和完成时间（分发人ID和分发时间） -->
                  <div class="flex items-center gap-4 text-sm text-gray-700 font-bold">
                    <span>负责人：{{ order.distributorID }}</span>
                    <span>完成时间：{{ order.distributeTime }}</span>
                  </div>
                </div>
              </div>
              <div class="flex items-center justify-between rounded px-6 py-4"
                :class="order.status === '待分发' ? 'bg-gray-50' : 'bg-gray-100 opacity-70'">
                <div class="grid grid-cols-2 gap-x-8 gap-y-4">
                  <!-- 任务优先级 -->
                  <div class="flex flex-col gap-1">
                    <span class="text-gray-700 font-semibold text-sm">任务优先级：</span>
                    <!-- 修改这里：根据工单状态显示不同的组件 -->
                    <template v-if="order.status === '待分发'">
                      <!-- 待分发状态：显示下拉选择框，默认值为工单已有的任务优先级 -->
                      <el-select v-model="leaderPriority" placeholder="请选择" size="default" class="min-w-[120px] w-full custom-bottom-border"
                        clearable>
                        <el-option label="紧急" value="紧急" />
                        <el-option label="一般" value="一般" />
                      </el-select>
                    </template>
                    <template v-else>
                      <!-- 进行中状态：显示只读输入框，显示工单的任务优先级 -->
                      <el-input :model-value="order.taskPriority" size="default" class="min-w-[120px] w-full custom-bottom-border" disabled />
                    </template>
                  </div>

                  <!-- 下一流程负责人 -->
                  <div class="flex flex-col gap-1">
                    <span class="text-gray-700 font-semibold text-sm">下一流程负责人：</span>
                    <el-select v-model="order.toDispatcherName" placeholder="请选择执行人ID" filterable clearable size="default"
                      class="min-w-[120px] w-full custom-bottom-border" :disabled="order.status !== '待分发'"
                      @visible-change="val => val && fetchExecutorList(order.modelID)">
                      <el-option v-for="item in executor" :key="item.id" :label="`${item.name} (${item.id})`"
                        :value="item.id" />
                    </el-select>
                  </div>
                </div>
                <div class="flex items-center gap-3">
                  <el-button type="success" size="default"
                    :disabled="order.status !== '待分发' || !leaderPriority || !order.toDispatcherName"
                    @click="handleDistribute(order)">
                    <i class="i-mdi-check-circle-outline mr-1" /> 同意
                  </el-button>
                  <el-button type="danger" size="default" :disabled="order.status !== '待分发'"
                    @click="handleReject(order)">
                    <i class="i-mdi-close-circle-outline mr-1" /> 拒绝
                  </el-button>
                </div>
              </div>
            </div>
            <!-- 任务审批 -->
            <div v-if="['待审批', '待分发', '进行中'].includes(order.status)  && getActiveTab(order.orderID) === 'approve'" title=""
              :collaspe="!expandedMap[order.orderID]" height="auto" class="w-full border-l border-b border-r  rounded-b-lg border-gray-300 p-4">
              <div>
                <div class="w-full flex items-center justify-between pr-4">
                  <div class="flex items-center">
                    <span class="text-lg text-blue-900 font-bold">任务审批</span>
                    <!-- 状态点 -->
                    <span v-if="order.status === '待审批'" class="ml-2 inline-block align-middle"
                      style="width: 12px;height: 12px;background: #f59e42;border-radius: 50%;" title="进行中" />
                    <span v-else-if="['待分发', '进行中', '已完成'].includes(order.status)"
                      class="ml-2 inline-block align-middle"
                      style="width: 12px;height: 12px;background: #22c55e;border-radius: 50%;" title="已完成" />
                    <span v-else-if="order.status === '已退回'" class="ml-2 inline-block align-middle"
                      style="width: 12px;height: 12px;background: #ef4444;border-radius: 50%;" title="已退回" />
                  </div>
                  <!-- 负责人和完成时间（审批人ID和审批时间） -->
                  <div class="flex items-center gap-4 text-sm text-gray-700 font-bold">
                    <span>负责人：{{ order.approverID }}</span>
                    <span>完成时间：{{ order.approveTime }}</span>
                  </div>
                </div>
              </div>
              <div class="flex items-start justify-between rounded px-6 py-4"
                :class="order.status === '待审批' ? 'bg-gray-50' : 'bg-gray-100 opacity-70'">
                <div class="grid grid-cols-3 gap-x-8 gap-y-4 flex-1">
                  <!-- 参考优先级 -->
                  <div class="flex flex-col gap-1">
                    <span class="text-gray-700 font-semibold text-sm">参考优先级：</span>
                    <!-- 修改这里：根据工单状态显示不同的组件 -->
                    <div v-if="order.status === '待审批'">
                      <!-- 待审批状态：显示下拉选择框，默认值为工单已有的参考优先级 -->
                      <el-select v-model="leaderPriority" placeholder="请选择" size="default" class="min-w-[120px] w-full"
                        clearable popper-class="custom-select-dropdown">
                        <el-option label="紧急" value="紧急" />
                        <el-option label="一般" value="一般" />
                      </el-select>
                    </div>
                    <div v-else>
                      <!-- 非待审批状态：显示只读输入框，显示工单的参考优先级 -->
                      <el-input :model-value="order.referencePriority" size="default" class="min-w-[120px] w-full"
                        disabled />
                    </div>
                  </div>

                  <!-- 下一流程负责人-分发人选择 -->
                  <div class="flex flex-col gap-1">
                    <span class="text-gray-700 font-semibold text-sm">下一流程负责人：</span>
                    <el-select v-model="order.distributorID" placeholder="请选择负责人ID" filterable clearable size="default"
                      class="min-w-[120px] w-full" :disabled="order.status !== '待审批'"
                      @visible-change="val => val && fetchDistributorList(order.modelID)" popper-class="custom-select-dropdown">
                      <el-option v-for="item in distributor" :key="item.id" :label="`${item.name} (${item.id})`"
                        :value="item.id" />
                    </el-select>
                  </div>

                  <!-- 预计发送时间，仅交付发送和版本迭代+交付发送类工单显示 -->
                  <div v-if="['交付发送', '版本迭代+交付发送'].includes(order.type)" class="flex flex-col gap-1">
                    <span class="text-gray-700 font-semibold text-sm">预计发送时间：</span>
                    <div v-if="order.status === '待审批'">
                      <el-date-picker v-model="order.targetDeliveryTime" type="date" placeholder="请选择日期"
                        format="YYYY-MM-DD" value-format="YYYY-MM-DD" class="min-w-[120px] w-full"
                        :disabled="order.status !== '待审批'" clearable 
                        :shortcuts="deliveryTimeShortcuts" />
                    </div>
                    <div v-else>
                      <span class="rounded bg-gray-100 px-2 py-1 text-blue-700 font-bold">
                        {{ order.targetDeliveryTime || '未填写' }}
                      </span>
                    </div>
                  </div>
                </div>
                <div class="flex items-center gap-3 ml-6 mt-8">
                  <el-button type="success" size="default"
                    :disabled="order.status !== '待审批' || !leaderPriority || !order.distributorID || (['交付发送', '版本迭代+交付发送'].includes(order.type) && !order.targetDeliveryTime)"
                    @click="handleApprove(order)">
                    <i class="i-mdi-check-circle-outline mr-1" /> 同意
                  </el-button>
                  <el-button type="danger" size="default" :disabled="order.status !== '待审批'"
                    @click="handleReject(order)">
                    <i class="i-mdi-close-circle-outline mr-1" /> 拒绝
                  </el-button>
                </div>
              </div>
            </div>
            <!-- 任务发起（始终显示在最下方） -->
            <div  v-if="getActiveTab(order.orderID) === 'initiate'" title="" :collaspe="!expandedMap[order.orderID]" height="auto" class="w-full border-l border-b border-r  rounded-b-lg border-gray-300 p-4">
              <div>
                <div class="w-full flex items-center justify-between pr-4">
                  <div class="flex items-center">
                    <span class="text-lg text-blue-900 font-bold">任务发起</span>
                    <span class="ml-2 inline-block align-middle"
                      style="width: 12px;height: 12px;background: #22c55e;border-radius: 50%;" title="已完成" />
                    <!-- 标签和内容分开显示，标签小且不加粗，内容正常 -->
                    <span class="ml-3 text-sm text-gray-500">模型：</span>
                    <span class="ml-1 text-black font-semibold">{{ order.modelID }}</span>
                    <span class="ml-3 text-sm text-gray-500">
                      {{ order.type === '交付发送' ? '发送版本：' : '基准版本：' }}
                    </span>
                    <span class="ml-1 text-black font-semibold">{{ order.modelVersionID }}</span>
                    <span v-if="order.completeModelVersion" class="ml-3 text-sm text-gray-500">
                      Matlab版本号：
                    </span>
                    <span v-if="order.completeModelVersion" class="ml-1 text-black font-semibold">
                      {{ order.completeModelVersion }}
                    </span>
                  </div>
                  <div class="flex items-center gap-4 text-sm text-gray-700 font-bold">
                    <span>负责人：{{ order.promoterID }}</span>
                    <span>完成时间：{{ order.startTime }}</span>
                  </div>
                </div>
              </div>
              <div class="grid grid-cols-2 items-start gap-x-8 gap-y-4 rounded bg-gray-50 px-6 py-4">
                <!-- 基础信息 -->
                <div class="flex flex-col gap-2">
                  <span class="text-black font-semibold">工单：</span>
                  <input class="w-full  border-0 border-b border-gray-300 bg-gray-50  py-2 text-blue-700 font-bold  focus:outline-none focus:border-blue-500"
                    :value="order.orderID" readonly>
                </div>
                <div class="flex flex-col gap-2">
                  <span class="text-black font-semibold">工单类型：</span>
                  <input class="w-full  border-0 border-b border-gray-300 bg-gray-50  py-2 text-blue-700 font-bold  focus:outline-none focus:border-blue-500"
                    :value="order.type" readonly>
                </div>
                <div class="flex flex-col gap-2">
                  <span class="text-black font-semibold">发起人：</span>
                  <input class="w-full  border-0 border-b border-gray-300 bg-gray-50  py-2 text-blue-700 font-bold  focus:outline-none focus:border-blue-500"
                    :value="order.promoterID" readonly>
                </div>
                <div class="flex flex-col gap-2">
                  <span class="text-black font-semibold">发起时间：</span>
                  <input class="w-full  border-0 border-b border-gray-300 bg-gray-50  py-2 text-blue-700 font-bold  focus:outline-none focus:border-blue-500"
                    :value="order.startTime" readonly>
                </div>

                <!-- 问题复现类 -->
                <template v-if="order.type === '问题复现'">
                  <div class="flex flex-col gap-2">
                    <span class="text-black font-semibold">对应协调单：</span>
                    <input class="w-full border-0 border-b border-gray-300  bg-gray-50 py-2 text-sm text-black  focus:outline-none focus:border-blue-500"
                      :value="order.coordinationID || 'NA'" readonly>
                  </div>
                  <div class="col-span-1 flex flex-col gap-2">
                    <span class="text-black font-semibold">复现内容：</span>
                    <textarea
                      class="resize-none border-0 border-b border-gray-300 bg-gray-50  py-2 text-sm text-black focus:outline-none focus:border-blue-500"
                      :value="order.description" rows="2" readonly />
                  </div>
                  <div class="col-span-1 flex flex-col gap-1">
                    <span class="text-black font-semibold">创建备注：</span>
                    <textarea
                      class="resize-none border-0 border-b border-gray-300 bg-gray-50  py-2 text-sm text-black focus:outline-none focus:border-blue-500"
                      :value="order.createRemark || '无'" rows="2" readonly />
                  </div>
                  <div class="flex flex-col gap-2">
                    <span class="text-black font-semibold">复现参考文件：</span>
                    <template v-if="order.files && order.files.length">
                      <div class="flex flex-wrap gap-2">
                        <span v-for="(file, idx) in order.files" :key="idx"
                          class="flex cursor-pointer items-center text-blue-600 underline hover:text-blue-800"
                          @click="downloadFile(file.fileUrl, file.fileName)">
                          <i class="i-mdi-download mr-1" />
                          {{ file.fileName }}
                        </span>
                      </div>
                    </template>
                    <span v-else class="text-gray-400">无</span>
                  </div>
                  <div class="col-span-1 flex flex-col gap-2">
                    <span class="text-black font-semibold">创建备注：</span>
                    <textarea
                      class="resize-none border-0 border-b border-gray-300 bg-gray-50  py-2 text-sm text-black focus:outline-none focus:border-blue-500"
                      :value="order.createRemark || '无'" rows="2" readonly />
                  </div>
                  <div class="flex flex-col gap-2">
                    <span class="text-black font-semibold">审批人：</span>
                    <input class="w-full border-0 border-b border-gray-300  bg-gray-50 py-2 text-sm text-black  focus:outline-none focus:border-blue-500"
                      :value="order.approverID || 'NA'" readonly>
                  </div>
                </template>

                <!-- 版本迭代类 -->
                <template v-else-if="order.type === '版本迭代'">
                  <div class="flex flex-col gap-2">
                    <span class="text-black font-semibold">Matlab版本：</span>
                    <input class="w-full border-0 border-b border-gray-300  bg-gray-50 py-2 text-sm text-black  focus:outline-none focus:border-blue-500"
                      :value="order.completeModelVersion" readonly>
                  </div>
                  <div class="flex flex-col gap-2">
                    <span class="text-black font-semibold">对应协调单：</span>
                    <input class="w-full border-0 border-b border-gray-300  bg-gray-50 py-2 text-sm text-black  focus:outline-none focus:border-blue-500"
                      :value="order.coordinationID || 'NA'" readonly>
                  </div>
                  <div class="flex flex-col gap-2">
                    <span class="text-black font-semibold">更新内容：</span>
                    <textarea
                      class="resize-none border-0 border-b border-gray-300 bg-gray-50  py-2 text-sm text-black focus:outline-none focus:border-blue-500"
                      :value="order.updateNotes" rows="2" readonly />
                  </div>
                  <div class="flex flex-col gap-2">
                    <span class="text-black font-semibold">封装要求：</span>
                    <textarea
                      class="resize-none border-0 border-b border-gray-300 bg-gray-50  py-2 text-sm text-black focus:outline-none focus:border-blue-500"
                      :value="order.packageRequirement" rows="2" readonly />
                  </div>
                  <div class="flex flex-col gap-2">
                    <span class="text-black font-semibold">
                      接口与{{ order.modelVersionID || '基准版本' }}是否变化：
                    </span>
                    <input class="w-full border-0 border-b border-gray-300  bg-gray-50 py-2 text-sm text-black  focus:outline-none focus:border-blue-500"
                      :value="order.apiChanged" readonly>
                  </div>
                  <div class="flex flex-col gap-2">
                    <span class="text-black font-semibold">审批人：</span>
                    <input class="w-full border-0 border-b border-gray-300  bg-gray-50 py-2 text-sm text-black  focus:outline-none focus:border-blue-500"
                      :value="order.approverID || 'NA'" readonly>
                  </div>
                </template>

                <!-- 交付发送类 -->
                <template v-else-if="order.type === '交付发送'">
                  <div class="flex flex-col gap-2">
                    <span class="text-black font-semibold">目标客户：</span>
                    <input class="w-full border-0 border-b border-gray-300  bg-gray-50 py-2 text-sm text-black  focus:outline-none focus:border-blue-500"
                      :value="order.targetCustomer" readonly>
                  </div>
                  <div class="flex flex-col gap-2">
                    <span class="text-black font-semibold">CAE-IPT平台验证：</span>
                    <input class="w-full border-0 border-b border-gray-300  bg-gray-50 py-2 text-sm text-black  focus:outline-none focus:border-blue-500"
                      :value="order.isCAEChecked" readonly>
                  </div>
                  <div class="flex flex-col gap-2">
                    <span class="text-black font-semibold">包含敏感信息：</span>
                    <input class="w-full border-0 border-b border-gray-300  bg-gray-50 py-2 text-sm text-black  focus:outline-none focus:border-blue-500"
                      :value="order.hasSensitiveInfo" readonly>
                  </div>
                  <div class="col-span-1 flex flex-col gap-1">
                    <span class="text-black font-semibold">创建备注：</span>
                    <textarea
                      class="resize-none border-0 border-b border-gray-300 bg-gray-50  py-2 text-sm text-black focus:outline-none focus:border-blue-500"
                      :value="order.createRemark || '无'" rows="2" readonly />
                  </div>
                  <div class="flex flex-col gap-2">
                    <span class="text-black font-semibold">审批人：</span>
                    <input class="w-full border-0 border-b border-gray-300  bg-gray-50 py-2 text-sm text-black  focus:outline-none focus:border-blue-500"
                      :value="order.approverID || 'NA'" readonly>
                  </div>
                </template>

                <!-- 版本迭代+交付发送类 -->
                <template v-else-if="order.type === '版本迭代+交付发送'">
                  <div class="flex flex-col gap-2">
                    <span class="text-black font-semibold">Matlab版本：</span>
                    <input class="w-full border-0 border-b border-gray-300  bg-gray-50 py-2 text-sm text-black  focus:outline-none focus:border-blue-500"
                      :value="order.completeModelVersion" readonly>
                  </div>
                  <div class="flex flex-col gap-2">
                    <span class="text-black font-semibold">对应协调单：</span>
                    <input class="w-full border-0 border-b border-gray-300  bg-gray-50 py-2 text-sm text-black  focus:outline-none focus:border-blue-500"
                      :value="order.coordinationID || 'NA'" readonly>
                  </div>
                  <div class="flex flex-col gap-2">
                    <span class="text-black font-semibold">更新内容：</span>
                    <textarea
                      class="resize-none border-0 border-b border-gray-300 bg-gray-50  py-2 text-sm text-black focus:outline-none focus:border-blue-500"
                      :value="order.updateNotes" rows="2" readonly />
                  </div>
                  <div class="flex flex-col gap-2">
                    <span class="text-black font-semibold">封装要求：</span>
                    <textarea
                      class="resize-none border-0 border-b border-gray-300 bg-gray-50  py-2 text-sm text-black focus:outline-none focus:border-blue-500"
                      :value="order.packageRequirement" rows="2" readonly />
                  </div>
                  <div class="flex flex-col gap-2">
                    <span class="text-black font-semibold">
                      接口与{{ order.modelVersionID || '基准版本' }}是否变化：
                    </span>
                    <input class="w-full border-0 border-b border-gray-300  bg-gray-50 py-2 text-sm text-black  focus:outline-none focus:border-blue-500"
                      :value="order.apiChanged" readonly>
                  </div>
                  <div class="flex flex-col gap-2">
                    <span class="text-black font-semibold">目标客户：</span>
                    <input class="w-full border-0 border-b border-gray-300  bg-gray-50 py-2 text-sm text-black  focus:outline-none focus:border-blue-500"
                      :value="order.targetCustomer" readonly>
                  </div>
                  <div class="flex flex-col gap-2">
                    <span class="text-black font-semibold">CAE-IPT平台验证：</span>
                    <input class="w-full border-0 border-b border-gray-300  bg-gray-50 py-2 text-sm text-black  focus:outline-none focus:border-blue-500"
                      :value="order.isCAEChecked" readonly>
                  </div>
                  <div class="flex flex-col gap-2">
                    <span class="text-black font-semibold">包含敏感信息：</span>
                    <input class="w-full border-0 border-b border-gray-300  bg-gray-50 py-2 text-sm text-black  focus:outline-none focus:border-blue-500"
                      :value="order.hasSensitiveInfo" readonly>
                  </div>
                  <div class="col-span-1 flex flex-col gap-2">
                    <span class="text-black font-semibold">创建备注：</span>
                    <textarea
                      class="resize-none border-0 border-b border-gray-300 bg-gray-50  py-2 text-sm text-black focus:outline-none focus:border-blue-500"
                      :value="order.createRemark || '无'" rows="2" readonly />
                  </div>
                  <div class="flex flex-col gap-2">
                    <span class="text-black font-semibold">审批人：</span>
                    <input class="w-full border-0 border-b border-gray-300  bg-gray-50 py-2 text-sm text-black  focus:outline-none focus:border-blue-500"
                      :value="order.approverID || 'NA'" readonly>
                  </div>
                </template>

                <!-- 功能开发类 -->
                <template v-else-if="order.type === '功能开发'">
                  <!-- <div class="flex items-center gap-2">
                      <span class="w-32 text-black font-semibold">升级后模型版本：</span>
                      <input class="flex-1 border border-gray-200 rounded bg-gray-50 px-3 py-2 text-sm text-black" :value="order.completeModelVersion" readonly>
                    </div> -->
                  <div class="flex flex-col gap-2">
                    <span class="text-black font-semibold">功能描述：</span>
                    <textarea
                      class="resize-none border-0 border-b border-gray-300 bg-gray-50  py-2 text-sm text-black focus:outline-none focus:border-blue-500"
                      :value="order.featureDesc" rows="2" readonly />
                  </div>
                  <div class="col-span-1 flex flex-col gap-2">
                    <span class="text-black font-semibold">创建备注：</span>
                    <textarea
                      class="resize-none border-0 border-b border-gray-300 bg-gray-50  py-2 text-sm text-black focus:outline-none focus:border-blue-500"
                      :value="order.createRemark || '无'" rows="2" readonly />
                  </div>
                  <div class="flex flex-col gap-2">
                    <span class="text-black font-semibold">审批人：</span>
                    <input class="w-full border-0 border-b border-gray-300  bg-gray-50 py-2 text-sm text-black  focus:outline-none focus:border-blue-500"
                      :value="order.approverID || 'NA'" readonly>
                  </div>
                </template>

                <!-- 其他类 -->
                <template v-else-if="order.type === '其他'">
                  <div class="flex flex-col gap-2">
                    <span class="text-black font-semibold">内容描述：</span>
                    <textarea
                      class="resize-none border-0 border-b border-gray-300 bg-gray-50  py-2 text-sm text-black focus:outline-none focus:border-blue-500"
                      :value="order.contentDesc" rows="2" readonly />
                  </div>
                  <div class="col-span-1 flex flex-col gap-2">
                    <span class="text-black font-semibold">创建备注：</span>
                    <textarea
                      class="resize-none border-0 border-b border-gray-300 bg-gray-50  py-2 text-sm text-black focus:outline-none focus:border-blue-500"
                      :value="order.createRemark || '无'" rows="2" readonly />
                  </div>
                  <div class="flex flex-col gap-2">
                    <span class="text-black font-semibold">审批人：</span>
                    <input class="w-full border-0 border-b border-gray-300  bg-gray-50 py-2 text-sm text-black  focus:outline-none focus:border-blue-500"
                      :value="order.approverID || 'NA'" readonly>
                  </div>
                </template>
              </div>
            </div>
            <div class="mt-6" />
          </FaPageMain>
        </div>
        <!-- 拒绝原因弹窗 -->
        <el-dialog v-model="rejectDialogVisible" title="填写拒绝原因" width="400px" :close-on-click-modal="false">
          <el-form>
            <el-form-item label="拒绝原因" required>
              <el-input v-model="rejectReasonInput" type="textarea" :rows="3" placeholder="请填写拒绝原因" maxlength="200"
                show-word-limit />
            </el-form-item>
          </el-form>
          <template #footer>
            <el-button @click="rejectDialogVisible = false">
              取消
            </el-button>
            <el-button type="primary" :disabled="!rejectReasonInput" @click="confirmReject">
              确定
            </el-button>
          </template>
        </el-dialog>
        <!-- 批量审批弹窗 -->
        <el-dialog v-model="batchApproveDialogVisible" title="批量审批" width="50vw" height="40vh"
          :close-on-click-modal="false">
          <el-form>
            <el-form-item label="参考优先级" required>
              <el-select v-model="batchLeaderPriority" placeholder="请选择">
                <el-option label="紧急" value="紧急" />
                <el-option label="一般" value="一般" />
              </el-select>
            </el-form-item>
            <el-form-item label="下一流程负责人ID" required>
              <el-select v-model="batchDistributorID" placeholder="请选择分发人ID" filterable clearable class="w-full"
                @visible-change="val => val && fetchDistributorList(getBatchModelId())">
                <el-option v-for="item in distributor" :key="item.id" :label="`${item.name} (${item.id})`"
                  :value="item.id" />
              </el-select>
            </el-form-item>
            <!-- 新增：预计发送时间，仅交付发送和版本迭代+交付发送类工单显示 -->
            <el-form-item v-if="userOrders
              .filter(o => selectedOrderIds.includes(o.orderID))
              .some(o => ['交付发送', '版本迭代+交付发送'].includes(o.type))" label="预计发送时间" required>
              <el-date-picker v-model="batchTargetDeliveryTime" type="date" placeholder="请选择日期" format="YYYY-MM-DD"
                value-format="YYYY-MM-DD" class="min-w-[120px] w-full" clearable 
                :shortcuts="deliveryTimeShortcuts" />
            </el-form-item>
            <!-- 工单信息预览 -->
            <el-form-item label="已选工单" label-width="80px">
              <div style="width: 100%;max-height: 36vh; overflow-y: auto;">
                <table style="width: 100%; font-size: 14px; border-collapse: separate;">
                  <thead>
                    <tr>
                      <th style=" position: sticky; top: 0; z-index: 2;text-align: left; background: #f3f4f6;">
                        工单号
                      </th>
                      <th style=" position: sticky; top: 0; z-index: 2;text-align: left; background: #f3f4f6;">
                        模型
                      </th>
                      <th style=" position: sticky; top: 0; z-index: 2;text-align: left; background: #f3f4f6;">
                        基准版本
                      </th>
                    </tr>
                  </thead>
                  <tbody>
                    <tr v-for="order in userOrders.filter(o => selectedOrderIds.includes(o.orderID))"
                      :key="order.orderID">
                      <td>{{ order.orderID }}</td>
                      <td>{{ order.modelID }}</td>
                      <td>{{ order.modelVersionID }}</td>
                    </tr>
                  </tbody>
                </table>
                <div v-if="selectedOrderIds.length === 0" class="mt-2 text-xs text-gray-400">
                  暂无选中工单
                </div>
              </div>
            </el-form-item>
          </el-form>
          <template #footer>
            <el-button @click="() => { batchApproveDialogVisible = false; selectedOrderIds = [] }">
              取消
            </el-button>
            <el-button type="primary"
              :disabled="!batchLeaderPriority || !batchDistributorID || (userOrders.filter(o => selectedOrderIds.includes(o.orderID)).some(o => ['交付发送', '版本迭代+交付发送'].includes(o.type)) && !batchTargetDeliveryTime)"
              @click="confirmBatchApprove">
              确定
            </el-button>
          </template>
        </el-dialog>

        <!-- 批量分发弹窗 -->
        <el-dialog v-model="batchDistributeDialogVisible" title="批量分发" width="50vw" height="40vh"
          :close-on-click-modal="false">
          <el-form>
            <el-form-item label="任务优先级" required>
              <el-select v-model="batchTaskPriority" placeholder="请选择">
                <el-option label="紧急" value="紧急" />
                <el-option label="一般" value="一般" />
              </el-select>
            </el-form-item>
            <el-form-item label="下一流程负责人ID" required>
              <el-select v-model="batchExecutorID" placeholder="请选择执行人ID" filterable clearable style="width: 100%;"
                @visible-change="val => val && fetchExecutorList(getBatchModelId())">
                <el-option v-for="item in executor" :key="item.id" :label="`${item.name} (${item.id})`"
                  :value="item.id" />
              </el-select>
            </el-form-item>
            <!-- 新增：批量分发工单信息预览 -->
            <el-form-item label="已选工单" label-width="80px">
              <div style="width: 100%;max-height: 36vh; overflow-y: auto;">
                <table style="width: 100%; font-size: 14px; border-collapse: separate;">
                  <thead>
                    <tr>
                      <th style="position: sticky; top: 0; z-index: 2;text-align: left; background: #f3f4f6;">
                        工单号
                      </th>
                      <th style="position: sticky; top: 0; z-index: 2;text-align: left; background: #f3f4f6;">
                        模型
                      </th>
                      <th style="position: sticky; top: 0; z-index: 2;text-align: left; background: #f3f4f6;">
                        基准版本
                      </th>
                    </tr>
                  </thead>
                  <tbody>
                    <tr v-for="order in userOrders.filter(o => selectedOrderIds.includes(o.orderID))"
                      :key="order.orderID">
                      <td>{{ order.orderID }}</td>
                      <td>{{ order.modelID }}</td>
                      <td>{{ order.modelVersionID }}</td>
                    </tr>
                  </tbody>
                </table>
                <div v-if="selectedOrderIds.length === 0" class="mt-2 text-xs text-gray-400">
                  暂无选中工单
                </div>
              </div>
            </el-form-item>
          </el-form>
          <template #footer>
            <el-button @click="() => { batchDistributeDialogVisible = false; selectedOrderIds = [] }">
              取消
            </el-button>
            <el-button type="primary" :disabled="!batchTaskPriority || !batchExecutorID"
              @click="confirmBatchDistribute">
              确定
            </el-button>
          </template>
        </el-dialog>

        <!-- 完成工单确认弹窗 -->
        <el-dialog v-model="finishConfirmDialogVisible" :title="`确认提交工单 #${confirmOrder?.orderID}`" width="600px"
          :close-on-click-modal="false">
          <div v-if="confirmOrder" class="space-y-4">

            <!-- 完成内容信息 -->
            <div class="rounded bg-blue-50 p-4">
              <h4 class="mb-3 text-lg text-gray-800 font-semibold">
                完成内容确认
              </h4>

              <!-- 问题复现类工单 -->
              <template v-if="confirmOrder.type === '问题复现'">
                <div class="text-sm space-y-3">
                  <div>
                    <span class="text-gray-600 font-medium">复现现象：</span>
                    <div class="mt-1 border rounded bg-white p-2">
                      {{ confirmOrder.finishPhenomenon || '未填写' }}
                    </div>
                  </div>
                  <div>
                    <span class="text-gray-600 font-medium">备注：</span>
                    <div class="mt-1 border rounded bg-white p-2">
                      {{ confirmOrder.finishRemark || '未填写' }}
                    </div>
                  </div>
                </div>
              </template>

              <!-- 版本迭代类工单 -->
              <template v-else-if="confirmOrder.type === '版本迭代'">
                <template v-if="confirmOrder.statusTodo === '待封装'">
                  <div class="text-sm space-y-3">
                    <div>
                      <span class="text-gray-600 font-medium">升级后版本：</span>
                      <div class="mt-1 border rounded bg-white p-2 text-blue-700 font-bold">
                        {{ confirmOrder.finishModelVersion || '未填写' }}
                      </div>
                    </div>
                    <div>
                      <span class="text-gray-600 font-medium">封装备注：</span>
                      <div class="mt-1 border rounded bg-white p-2">
                        {{ confirmOrder.packageRemark || '未填写' }}
                      </div>
                    </div>
                  </div>
                </template>
              </template>

              <!-- 交付发送类工单 -->
              <template v-else-if="confirmOrder.type === '交付发送'">
                <template v-if="confirmOrder.statusTodo === '待发送'">
                  <div>
                    <span class="text-gray-600 font-medium">是否加密：</span>
                    <span class="ml-2 rounded px-2 py-1 text-white font-bold"
                      :class="confirmOrder.isEncrypted === '是' ? 'bg-red-500' : 'bg-green-500'">
                      {{ confirmOrder.isEncrypted || '未选择' }}
                    </span>
                  </div>
                  <template v-if="confirmOrder.isEncrypted === '是'">
                    <div>
                      <span class="text-gray-600 font-medium">授权ID：</span>
                      <div class="mt-1 border rounded bg-white p-2 text-blue-700 font-bold">
                        {{ confirmOrder.finishAuthId || '未填写' }}
                      </div>
                    </div>
                  </template>
                  <div>
                    <span class="text-gray-600 font-medium">发送客户：</span>
                    <div class="mt-1 border rounded bg-white p-2 text-blue-700 font-bold">
                      {{ confirmOrder.targetCustomer || '' }}
                    </div>
                  </div>
                  <div>
                    <span class="text-gray-600 font-medium">发送备注：</span>
                    <div class="mt-1 border rounded bg-white p-2">
                      {{ confirmOrder.sendRemark || '未填写' }}
                    </div>
                  </div>
                </template>
                <template v-else-if="confirmOrder.statusTodo === '待加密'">
                  <div class="text-sm space-y-3">
                    <div>
                      <span class="text-gray-600 font-medium">发送客户：</span>
                      <div class="mt-1 border rounded bg-white p-2 text-blue-700 font-bold">
                        {{ confirmOrder.targetCustomer || '' }}
                      </div>
                    </div>
                    <div>
                      <span class="text-gray-600 font-medium">是否加密：</span>
                      <span class="ml-2 rounded px-2 py-1 text-white font-bold"
                        :class="confirmOrder.isEncrypted === '是' ? 'bg-red-500' : 'bg-green-500'">
                        {{ confirmOrder.isEncrypted || '未选择' }}
                      </span>
                    </div>
                    <template v-if="confirmOrder.isEncrypted === '是'">
                      <div>
                        <span class="text-gray-600 font-medium">外壳号：</span>
                        <div class="mt-1 border rounded bg-white p-2 text-blue-700 font-bold">
                          {{ confirmOrder.finishShellNo || '未填写' }}
                        </div>
                      </div>
                      <div>
                        <span class="text-gray-600 font-medium">授权ID：</span>
                        <div class="mt-1 border rounded bg-white p-2 text-blue-700 font-bold">
                          {{ confirmOrder.finishAuthId || '未填写' }}
                        </div>
                      </div>
                    </template>
                    <div>
                      <span class="text-gray-600 font-medium">加密备注：</span>
                      <div class="mt-1 border rounded bg-white p-2">
                        {{ confirmOrder.encryptedRemark || '未填写' }}
                      </div>
                    </div>
                    <div>
                      <span class="text-gray-600 font-medium">发送人：</span>
                      <div class="mt-1 border rounded bg-white p-2">
                        {{ confirmOrder.sendExecutorID || '未选择' }}
                      </div>
                    </div>
                  </div>
                </template>
              </template>

              <!-- 版本迭代+交付发送类工单 -->
              <template v-else-if="confirmOrder.type === '版本迭代+交付发送'">
                <template v-if="confirmOrder.statusTodo === '待封装'">
                  <div class="text-sm space-y-3">
                    <div>
                      <span class="text-gray-600 font-medium">升级后版本：</span>
                      <div class="mt-1 border rounded bg-white p-2 text-blue-700 font-bold">
                        {{ confirmOrder.finishModelVersion || '未填写' }}
                      </div>
                    </div>
                    <div>
                      <span class="text-gray-600 font-medium">封装备注：</span>
                      <div class="mt-1 border rounded bg-white p-2">
                        {{ confirmOrder.packageRemark || '未填写' }}
                      </div>
                    </div>
                  </div>
                </template>
                <template v-else-if="confirmOrder.statusTodo === '待加密'">
                  <div class="text-sm space-y-3">
                    <div>
                      <span class="text-gray-600 font-medium">发送客户：</span>
                      <div class="mt-1 border rounded bg-white p-2 text-blue-700 font-bold">
                        {{ confirmOrder.targetCustomer || '' }}
                      </div>
                    </div>
                    <div>
                      <span class="text-gray-600 font-medium">发送版本：</span>
                      <div class="mt-1 border rounded bg-white p-2 text-blue-700 font-bold">
                        {{ confirmOrder.finishModelVersion || '' }}
                      </div>
                    </div>
                    <div>
                      <span class="text-gray-600 font-medium">是否加密：</span>
                      <span class="ml-2 rounded px-2 py-1 text-white font-bold"
                        :class="confirmOrder.isEncrypted === '是' ? 'bg-red-500' : 'bg-green-500'">
                        {{ confirmOrder.isEncrypted || '未选择' }}
                      </span>
                    </div>
                    <template v-if="confirmOrder.isEncrypted === '是'">
                      <div>
                        <span class="text-gray-600 font-medium">外壳号：</span>
                        <div class="mt-1 border rounded bg-white p-2 text-blue-700 font-bold">
                          {{ confirmOrder.finishShellNo || '未填写' }}
                        </div>
                      </div>
                      <div>
                        <span class="text-gray-600 font-medium">授权ID：</span>
                        <div class="mt-1 border rounded bg-white p-2 text-blue-700 font-bold">
                          {{ confirmOrder.finishAuthId || '未填写' }}
                        </div>
                      </div>
                    </template>
                    <div>
                      <span class="text-gray-600 font-medium">加密备注：</span>
                      <div class="mt-1 border rounded bg-white p-2">
                        {{ confirmOrder.encryptedRemark || '未填写' }}
                      </div>
                    </div>
                  </div>
                </template>
                <template v-else-if="confirmOrder.statusTodo === '待发送'">
                  <div>
                    <span class="text-gray-600 font-medium">是否加密：</span>
                    <span class="ml-2 rounded px-2 py-1 text-white font-bold"
                      :class="confirmOrder.isEncrypted === '是' ? 'bg-red-500' : 'bg-green-500'">
                      {{ confirmOrder.isEncrypted || '未选择' }}
                    </span>
                  </div>
                  <template v-if="confirmOrder.isEncrypted === '是'">
                    <div>
                      <span class="text-gray-600 font-medium">授权ID：</span>
                      <div class="mt-1 border rounded bg-white p-2 text-blue-700 font-bold">
                        {{ confirmOrder.finishAuthId || '未填写' }}
                      </div>
                    </div>
                  </template>
                  <div>
                    <span class="text-gray-600 font-medium">发送客户：</span>
                    <div class="mt-1 border rounded bg-white p-2 text-blue-700 font-bold">
                      {{ confirmOrder.targetCustomer || '' }}
                    </div>
                  </div>
                  <div>
                    <span class="text-gray-600 font-medium">发送版本：</span>
                    <div class="mt-1 border rounded bg-white p-2 text-blue-700 font-bold">
                      {{ confirmOrder.finishModelVersion || '' }}
                    </div>
                  </div>
                  <div>
                    <span class="text-gray-600 font-medium">发送备注：</span>
                    <div class="mt-1 border rounded bg-white p-2">
                      {{ confirmOrder.sendRemark || '未填写' }}
                    </div>
                  </div>
                </template>
              </template>

              <!-- 功能开发类工单 -->
              <template v-else-if="confirmOrder.type === '功能开发'">
                <div class="text-sm space-y-3">
                  <div>
                    <span class="text-gray-600 font-medium">完成后版本：</span>
                    <div class="mt-1 border rounded bg-white p-2 text-blue-700 font-bold">
                      {{ confirmOrder.finishModelVersion || '未填写' }}
                    </div>
                  </div>
                  <div>
                    <span class="text-gray-600 font-medium">完成功能描述：</span>
                    <div class="mt-1 border rounded bg-white p-2">
                      {{ confirmOrder.finishFeatureDesc || '未填写' }}
                    </div>
                  </div>
                </div>
              </template>

              <!-- 其他类工单 -->
              <template v-else-if="confirmOrder.type === '其他'">
                <div class="text-sm space-y-3">
                  <div>
                    <span class="text-gray-600 font-medium">备注：</span>
                    <div class="mt-1 border rounded bg-white p-2">
                      {{ confirmOrder.finishRemarkOther || '未填写' }}
                    </div>
                  </div>
                </div>
              </template>
            </div>

            <!-- 提示信息 -->
            <div class="border border-yellow-200 rounded bg-yellow-50 p-3">
              <div class="flex items-center">
                <i class="i-mdi-alert mr-2 text-yellow-600" />
                <span class="text-sm text-yellow-800">
                  <strong>提示：</strong>提交后工单状态将变化，请确认以上信息无误后再提交。
                </span>
              </div>
            </div>
          </div>

          <template #footer>
            <div class="flex justify-end gap-3">
              <el-button @click="finishConfirmDialogVisible = false">
                取消
              </el-button>
              <el-button type="primary" @click="confirmFinishOrder">
                确认提交
              </el-button>
            </div>
          </template>
        </el-dialog>

        <!-- 授权ID+外壳号选择弹窗示例 -->
        <el-dialog v-model="authSelectDialogVisible" title="选择授权ID和外壳号" width="70vw" height="60vh"
          :close-on-click-modal="false" @close="handleCloseAuthSelect">
          <div v-if="customerAuthIds.length > 0" style="max-height: 50vh; overflow-y: auto;">
            <div v-for="group in groupAuthShells(customerAuthIds)" :key="group.authId"
              class="mb-4 border border-blue-300 rounded-lg bg-blue-50 hover:bg-blue-100 cursor-pointer transition-all"
              @click="selectAuthIdShell(group.authId)" style="padding: 16px;">
              <div class="flex items-center mb-2">
                <span class="text-lg text-blue-700 font-bold mr-4">授权ID：{{ group.authId }}</span>
                <span class="text-gray-500 text-sm">（点击区域选择）</span>
              </div>
              <table class="w-full" style="font-size: 15px; table-layout: fixed;">
                <thead>
                  <tr class="bg-gray-50">
                    <th style="width: 25%; text-align: left;">外壳号</th>
                    <th style="width: 25%; text-align: left;">剩余天数</th>
                    <th style="width: 25%; text-align: left;">设备类型</th>
                    <th style="width: 25%; text-align: left;">备注</th>
                  </tr>
                </thead>
                <tbody>
                  <tr v-for="shell in group.shells" :key="shell.shellNumber">
                    <td class="border-b border-gray-200 px-3 py-2"
                      style="max-width: 120px; overflow: hidden; text-overflow: ellipsis; white-space: nowrap; text-align: left;">
                      <span class="text-green-700">{{ shell.shellNumber }}</span>
                    </td>
                    <td class="border-b border-gray-200 px-3 py-2" style="text-align: left;">
                      <span :class="shell.remainingDaysColor">{{ shell.remainingDays }}</span>
                    </td>
                    <td class="border-b border-gray-200 px-3 py-2" style="text-align: left;">
                      {{ shell.deviceType }}
                    </td>
                    <td class="border-b border-gray-200 px-3 py-2" style="text-align: left;">
                      {{ shell.authNote || '无' }}
                    </td>
                  </tr>
                </tbody>
              </table>
            </div>
          </div>
          <div v-else class="py-8 text-center text-gray-500">
            <el-icon class="mb-2 text-4xl">
              <DocumentDelete />
            </el-icon>
            <div>暂无可用的授权ID和外壳号</div>
          </div>
          <template #footer>
            <div class="flex justify-end">
              <el-button @click="handleCloseAuthSelect">
                取消
              </el-button>
            </div>
          </template>
        </el-dialog>

      </template>
    </el-skeleton>
  </div>
</template>

<style scoped>
/* 版本输入容器 - 验证码样式 */
.version-input-container {
  display: flex;
  flex-wrap: wrap;
  /* 添加换行支持 */
  gap: 6px;
  align-items: center;
  justify-content: flex-start;
}

/* 版本部分样式 */
.version-part {
  display: flex;
  flex-shrink: 0;
  /* 防止收缩 */
  align-items: center;
  justify-content: center;
  width: 32px;
  height: 32px;
  font-family: "Courier New", monospace;
  font-size: 16px;
  font-weight: bold;
  background-color: #f9fafb;
  border: 2px solid #e5e7eb;
  border-radius: 6px;
}

/* 静态部分（点号） */
.version-part.static {
  color: #6b7280;
  background-color: #f3f4f6;
  border-color: #d1d5db;
}

/* 只读部分（已有的数字） */
.version-part.readonly {
  color: #374151;
  background-color: #e5e7eb;
  border-color: #d1d5db;
}

/* 用户输入框 */
.version-input {
  flex-shrink: 0;
  /* 防止收缩 */
  width: 40px;
}

.version-input :deep(.el-input__wrapper) {
  height: 32px;
  background-color: #fff;
  border: 2px solid #3b82f6;
  border-radius: 6px;
  box-shadow: 0 0 0 2px rgb(59 130 246 / 10%);
  transition: all 0.2s ease;
}

.version-input :deep(.el-input__wrapper:hover) {
  border-color: #2563eb;
  box-shadow: 0 0 0 2px rgb(37 99 235 / 15%);
}

.version-input :deep(.el-input__wrapper.is-focus) {
  border-color: #1d4ed8;
  box-shadow: 0 0 0 2px rgb(29 78 216 / 20%);
}

.version-input :deep(.el-input__inner) {
  padding: 0;
  font-family: "Courier New", monospace;
  font-size: 16px;
  font-weight: bold;
  color: #1f2937;
  text-align: center;
}

.version-input :deep(.el-input__inner::placeholder) {
  font-weight: normal;
  color: #9ca3af;
}

/* 禁用状态 */
.version-input.is-disabled :deep(.el-input__wrapper) {
  cursor: not-allowed;
  background-color: #f9fafb;
  border-color: #e5e7eb;
}

.version-input.is-disabled :deep(.el-input__inner) {
  color: #9ca3af;
  cursor: not-allowed;
}

/* 响应式调整 */
@media (width <=640px) {
  .version-input-container {
    gap: 4px;
  }

  .version-part {
    width: 28px;
    height: 28px;
    font-size: 14px;
  }

  .version-input {
    width: 32px;
  }

  .version-input :deep(.el-input__wrapper) {
    height: 28px;
  }

  .version-input :deep(.el-input__inner) {
    font-size: 14px;
  }
}
</style>
