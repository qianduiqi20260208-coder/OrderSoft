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
// 流转相关数据结构
interface TransferInfo {
  transferExecutorID: string
  transferReason: string
  transferTime: string
}
// 工单数据结构定义，统一用 interface 进行类型约束
interface OrderItem {
  // 工单类型
  type: '问题复现' | '版本迭代' | '交付发送' | '版本迭代+交付发送' | '功能开发' | '其他'
  // 工单状态
  status: '草稿' | '待审批' | '待分发' | '进行中' | '已完成' | '已退回'
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
  approveTime?: string // 审批时间
  distributeTime?: string // 分发时间
  rejectReason?: string // 拒绝原因
  executorID?: string // 执行人ID
  finishTime?: string // 完成时间

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
  finishShellNo?: string // 外壳号（字母+数字）

  // 功能开发类（完成）
  finishModelVersionId?: string // 完成后模型版本ID
  finishFeatureDesc?: string // 完成功能描述

  // 其他类（完成）
  finishRemarkOther?: string // 备注（完成）

  // 流转相关内容
  // 历史流转信息（只读，后端返回）
  // transferReason?: string
  // transferTime?: string
  // transferExecutorID?: string
  transfers?: TransferInfo[] // 多次流转记录

  // 前端本次编辑用（不提交时不影响历史）
  transferReasonEdit?: string
  transferExecutorIDEdit?: string
  transferTimeEdit?: string

  finishModelVersionNumber?: string
  finishModelVersionLetter?: string
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

// -----------事件处理函数--------------
// 一键展开/收起
// function expandOrder(orderId: string, expand: boolean) {
//   expandedMap.value[orderId] = expand

//   if (expand) {
//     const order = userOrders.value.find(o => o.orderID === orderId)
//     if (order && order.status === '进行中' && order.completeModelVersion) {
//       orderApi.fetchUsedModelVersion(order.modelID, order.completeModelVersion)
//         .then((res) => {
//           // 假设后端返回 { usedModelVersions: ['xxx'] }
//           if (res?.data?.usedModelVersions) {
//             order.usedModelVersions = res.data.usedModelVersions || []
//           }
//         })
//         .catch((err) => {
//           ElMessage.error('获取新版本号失败')
//           console.error(err)
//         })
//     }
//   }
// }

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
// ------------页面设置------------
  // 设置参考优先级
  order.referencePriority = leaderPriority.value
  // 工单状态修改为待分发
  order.status = '待分发'
  // 设置审批领导ID和审批时间
  order.approverID = currentUserId // 当前登录用户ID
  const now = new Date()
  order.approveTime = `${now.getFullYear()}-${(now.getMonth() + 1).toString().padStart(2, '0')}-${now.getDate().toString().padStart(2, '0')} ${now.getHours().toString().padStart(2, '0')}:${now.getMinutes().toString().padStart(2, '0')}:${now.getSeconds().toString().padStart(2, '0')}`
  // 清空优先级选择
  leaderPriority.value = ''
  // ------------后端设置------------
  // 调用后端接口保存审批结果
  const res = await orderApi.approveOrder({
    orderID: order.orderID, // 工单ID
    status: order.status, // 工单状态（审批后变为待分发）
    approverID: String(order.approverID ?? ''), // 审批人ID（当前用户）强制转换为String后发送后端
    approveTime: order.approveTime, // 审批通过时间（系统自动获取）
    referencePriority: order.referencePriority, // 参考优先级（审批时设置）
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
    status: order.status, // 工单状态（分发后变为进行中）
    distributorID: String(order.distributorID ?? ''), // 分发人ID
    distributeTime: order.distributeTime, // 分发时间（系统自动获取）
    taskPriority: order.taskPriority, // 任务优先级
    executorID: String(order.executorID ?? ''), // 执行人ID
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

  // 如果选择不加密，清空外壳号和授权ID字段
  if (confirmOrder.value.isEncrypted === '否') {
    confirmOrder.value.finishShellNo = ''
    confirmOrder.value.finishAuthId = ''
  }

  // 设置状态为已完成，记录完成时间
  confirmOrder.value.status = '已完成'
  confirmOrder.value.finishTime = new Date().toISOString().slice(0, 19).replace('T', ' ')
  confirmOrder.value.executorID = currentUserId // 增加完成人ID（当前用户）

  // 根据工单类型分别调用后端接口
  let res
  if (confirmOrder.value.type === '问题复现') {
    res = await orderApi.finishProblemOrder({
      orderID: confirmOrder.value.orderID,
      status: confirmOrder.value.status,
      finishTime: confirmOrder.value.finishTime,
      finishPhenomenon: confirmOrder.value.finishPhenomenon ?? '',
      finishRemark: confirmOrder.value.finishRemark ?? '',
      executorID: confirmOrder.value.executorID ?? '',
    })
  }
  else if (confirmOrder.value.type === '版本迭代') {
    res = await orderApi.finishIterOrder({
      orderID: confirmOrder.value.orderID,
      modelID: confirmOrder.value.modelID,
      status: confirmOrder.value.status,
      finishTime: confirmOrder.value.finishTime,
      finishModelVersion: confirmOrder.value.finishModelVersion ?? '',
      finishRemark: confirmOrder.value.finishRemark ?? '',
      executorID: confirmOrder.value.executorID ?? '',
    })
  }
  else if (confirmOrder.value.type === '交付发送') {
    res = await orderApi.finishDeliverOrder({
      orderID: confirmOrder.value.orderID,
      status: confirmOrder.value.status,
      finishTime: confirmOrder.value.finishTime,
      isEncrypted: confirmOrder.value.isEncrypted ?? '',
      finishAuthId: confirmOrder.value.finishAuthId ?? '',
      finishShellNo: confirmOrder.value.finishShellNo ?? '',
      finishRemark: confirmOrder.value.finishRemark ?? '',
      executorID: confirmOrder.value.executorID ?? '',
    })
  }
  else if (confirmOrder.value.type === '版本迭代+交付发送') {
    res = await orderApi.finishIterDeliverOrder({
      orderID: confirmOrder.value.orderID,
      status: confirmOrder.value.status,
      modelID: confirmOrder.value.modelID,
      finishTime: confirmOrder.value.finishTime,
      finishModelVersion: confirmOrder.value.finishModelVersion ?? '',
      isEncrypted: confirmOrder.value.isEncrypted ?? '',
      finishAuthId: confirmOrder.value.finishAuthId ?? '',
      finishShellNo: confirmOrder.value.finishShellNo ?? '',
      finishRemark: confirmOrder.value.finishRemark ?? '',
      executorID: confirmOrder.value.executorID ?? '',
    })
  }
  else if (confirmOrder.value.type === '功能开发') {
    res = await orderApi.finishDevOrder({
      orderID: confirmOrder.value.orderID,
      status: confirmOrder.value.status,
      modelID: confirmOrder.value.modelID,
      modelVersion: confirmOrder.value.modelVersionID,
      finishTime: confirmOrder.value.finishTime,
      finishModelVersionId: confirmOrder.value.finishModelVersionId ?? '',
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
        // 选择加密：需要填写外壳号、授权ID和备注
        return !!order.finishAuthId && !!order.finishShellNo && !!order.isEncrypted
      }
      else if (order.isEncrypted === '否') {
        // 选择不加密：只需要填写备注
        return !!order.isEncrypted
      }
      else {
        // 未选择是否加密
        return false
      }
    case '版本迭代+交付发送':
      // 同样修改版本迭代+交付发送的逻辑
      if (order.isEncrypted === '是') {
        // 选择加密：需要填写模型版本、外壳号、授权ID和备注
        return !!order.finishModelVersion && !!order.finishAuthId && !!order.finishShellNo && !!order.isEncrypted
      }
      else if (order.isEncrypted === '否') {
        // 选择不加密：只需要填写模型版本和备注
        return !!order.finishModelVersion && !!order.isEncrypted
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

    // 处理返回的数据，将单个文件转换为文件数组
    const orders = res.data.list || []
    userOrders.value = orders.map((order: OrderItem) => {
      // 数据映射处理 - 将后端字段映射到前端期望的字段
      const mappedOrder: OrderItem = {
        // 基础字段
        orderID: order.orderID || '', // 工单ID
        type: order.type || '其他', // 工单类型
        status: order.status || '草稿', // 工单状态
        referencePriority: order.referencePriority || '', // 参考优先级
        taskPriority: order.taskPriority || '', // 任务优先级
        modelID: order.modelID || '', // 模型ID
        modelVersionID: order.modelVersionID || '', // 模型版本ID
        promoterID: order.promoterID || '', // 创建人ID
        startTime: order.startTime || '', // 创建时间
        completeModelVersion: order.completeModelVersion || '', // 完成的版本号

        // 审批分发相关
        approverID: order.approverID || '', // 审批人ID
        distributorID: order.distributorID || '', // 分发人ID
        executorID: order.executorID || '', // 执行人ID
        approveTime: order.approveTime || '', // 审批时间
        distributeTime: order.distributeTime || '', // 分发时间
        finishTime: order.finishTime || '', // 完成时间
        rejectReason: order.rejectReason || '', // 拒绝原因

        // 工单类型特定字段 - 创建阶段
        coordinationID: order.coordinationID || '', // 协调单号
        description: order.description || '', // 问题描述
        // 先初始化为空数组，后面会处理文件
        files: [],
        fileName: order.fileName || '', // 后端返回的文件名
        fileUrl: order.fileUrl || '', // 后端返回的文件URL
        hasAttachment: order.hasAttachment || false, // 是否有附件
        updateNotes: order.updateNotes || '', // 版本更新内容说明
        packageRequirement: order.packageRequirement || '', // 封装要求
        apiChanged: order.apiChanged || '', // 接口是否变化
        targetCustomer: order.targetCustomer || '', // 目标客户名称
        isCAEChecked: order.isCAEChecked || '', // 是否通过CAE
        hasSensitiveInfo: order.hasSensitiveInfo || '', // 是否包含敏感信息
        featureDesc: order.featureDesc || '', // 功能描述
        contentDesc: order.contentDesc || '', // 内容描述

        // 工单类型特定字段 - 完成阶段
        finishRemark: order.finishRemark || '', // 完成时备注
        finishPhenomenon: order.finishPhenomenon || '', // 复现现象描述
        finishModelVersion: order.finishModelVersion || '', // 升级后模型版本
        isEncrypted: order.isEncrypted || '', // 是否加密
        finishAuthId: order.finishAuthId || '', // 授权ID
        finishShellNo: order.finishShellNo || '', // 外壳号
        finishModelVersionId: order.finishModelVersionId || '', // 完成后模型版本ID
        finishFeatureDesc: order.finishFeatureDesc || '', // 完成后功能描述
        finishRemarkOther: order.finishRemarkOther || '', // 其他类工单完成时备注

        // 流转记录
        transfers: order.transfers || [], // 流转记录

        // 前端本次编辑用字段
        transferReasonEdit: '', // 流转原因编辑
        transferExecutorIDEdit: '', // 流转负责人编辑
        transferTimeEdit: '', // 流转时间编辑
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
  batchApproveDialogVisible.value = true
}

// 打开批量分发弹窗
function openBatchDistributeDialog() {
  batchTaskPriority.value = ''
  batchExecutorID.value = ''
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
async function confirmBatchApprove() {
  let successCount = 0
  for (const orderID of selectedOrderIds.value) {
    const order = userOrders.value.find(o => o.orderID === orderID)
    if (order && order.status === '待审批') {
      // 设置参考优先级和分发人ID
      order.referencePriority = batchLeaderPriority.value
      order.status = '待分发'
      order.distributorID = batchDistributorID.value
      order.approverID = currentUserId
      order.approveTime = new Date().toISOString().slice(0, 19).replace('T', ' ')
      // 调用单个审批接口
      const res = await orderApi.approveOrder({
        orderID: order.orderID,
        status: order.status,
        approverID: String(order.approverID ?? ''),
        approveTime: order.approveTime,
        referencePriority: order.referencePriority,
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
const shellNumbers = ref<string[]>([]) // 外壳号列表

// 修改获取外壳号列表的方法，根据目标客户获取
async function fetchShellNumbers(order?: OrderItem) {
  try {
    // 获取目标客户参数
    const targetCustomer = order?.targetCustomer

    if (!targetCustomer) {
      console.warn('未找到目标客户信息，无法获取外壳号列表')
      shellNumbers.value = []
      return
    }

    // 调用后端接口，传入目标客户参数
    const res = await orderApi.fetchShellNumbers(targetCustomer)
    shellNumbers.value = res?.data?.list || []
  }
  catch (error) {
    console.error('获取外壳号列表失败:', error)
    shellNumbers.value = []

    // 显示错误提示
    ElMessage.error('获取外壳号列表失败，请稍后重试')
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
    const response = await orderApi.downloadTicketFile(ticketId, fileName)

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
  const res = await orderApi.transferOrder({
    orderID: order.orderID, // 工单号
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

  order.transferReasonEdit = ''
  order.transferExecutorIDEdit = ''

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
}>>([])

// 处理授权ID下拉框点击事件
async function handleAuthIdSelectClick(order: OrderItem) {
  // 检查是否已选择外壳号
  if (!order.finishShellNo) {
    ElMessage.warning('请先选择外壳号')
    return
  }

  currentOrder.value = order

  try {
    // 调用获取授权ID列表的API，传入客户名和外壳号
    const res = await orderApi.fetchAuthIds(order.finishShellNo ?? '', order.targetCustomer ?? '')

    if (res?.data?.list) {
      // 处理返回的授权详情列表
      authDetailsForSelection.value = res.data.list.map((authDetail: any) => ({
        authId: authDetail.authId,
        remainingDays: calculateRemainingDays(authDetail.endDate),
        remainingDaysColor: getRemainingDaysColor(authDetail.endDate),
        deviceType: authDetail.deviceType,
      }))

      authSelectDialogVisible.value = true
    }
    else {
      ElMessage.error('获取授权ID列表失败')
    }
  }
  catch (error) {
    console.error('获取授权ID列表失败:', error)
    ElMessage.error('获取授权ID列表失败，请稍后重试')
  }
}

// 选择授权ID
function selectAuthId(authId: string) {
  if (currentOrder.value) {
    currentOrder.value.finishAuthId = authId
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
              <el-tooltip
                content="请选择待审批工单"
                placement="top"
                :disabled="selectedOrderIds.length > 0 && selectedOrderIds.every(id => {
                  const order = userOrders.find(o => o.orderID === id)
                  return order && order.status === '待审批'
                })"
              >
                <el-button
                  type="primary"
                  :disabled="
                    selectedOrderIds.length === 0
                      || selectedOrderIds.some(id => {
                        const order = userOrders.find(o => o.orderID === id)
                        return !order || order.status !== '待审批'
                      })
                  "
                  @click="openBatchApproveDialog"
                >
                  一键审批
                </el-button>
              </el-tooltip>
              <el-tooltip
                content="请选择待分发工单"
                placement="top"
                :disabled="selectedOrderIds.length > 0 && selectedOrderIds.every(id => {
                  const order = userOrders.find(o => o.orderID === id)
                  return order && order.status === '待分发'
                })"
              >
                <el-button
                  type="success"
                  :disabled="
                    selectedOrderIds.length === 0
                      || selectedOrderIds.some(id => {
                        const order = userOrders.find(o => o.orderID === id)
                        return !order || order.status !== '待分发'
                      })
                  "
                  @click="openBatchDistributeDialog"
                >
                  一键分发
                </el-button>
              </el-tooltip>
            </div>
          </div>
        </FaPageMain>
        <div class="space-y-4">
          <!-- 每个工单一个内容块 -->
          <FaPageMain
            v-for="order in userOrders"
            :key="order.orderID"
            :title="`工单#${order.orderID}`"
            :collaspe="!expandedMap[order.orderID]"
            height="auto"
            class="w-full"
          >
            <template #title>
              <div class="w-full flex items-center justify-between">
                <div>
                  <span class="block flex items-center text-lg text-black font-extrabold tracking-wide">
                    <!-- 工单前面加选择框 -->
                    <el-checkbox
                      :model-value="selectedOrderIds.includes(order.orderID)"
                      style=" margin-right: 12px;margin-left: 8px;"
                      @change="checked => handleSelectOrder(order.orderID, Boolean(checked))"
                    />
                    工单#{{ order.orderID }}
                    <span
                      class="ml-4 align-middle text-base font-bold"
                      :class="{
                        'text-blue-700': order.type === '问题复现',
                        'text-green-700': order.type === '版本迭代',
                        'text-yellow-700': order.type === '交付发送',
                        'text-purple-700': order.type === '版本迭代+交付发送',
                        'text-pink-700': order.type === '功能开发',
                        'text-gray-700': order.type === '其他',
                      }"
                    >
                      {{ order.type }}
                    </span>
                  </span>
                  <div class="mt-2 flex flex-wrap items-center gap-6 text-sm">
                    <!-- 通用展开/收起图标放在发起人ID左边，直接点击图标触发展开/收起 -->
                    <FaIcon
                      name="expand"
                      class="mr-0 cursor-pointer text-xl"
                      @click="expandOrder(order.orderID, !expandedMap[order.orderID])"
                    />
                    <span>
                      <i class="i-mdi-cube mr-1 text-blue-400" />
                      <span class="text-gray-600">模型：</span>
                      <span class="text-black font-bold">{{ order.modelID }}</span>
                    </span>
                    <span>
                      <i class="i-mdi-account mr-1 text-blue-400" />
                      <span class="text-gray-600">发起人：</span>
                      <span class="text-black font-bold">{{ order.promoterID }}</span>
                    </span>
                    <span>
                      <i class="i-mdi-calendar-clock mr-1 text-blue-400" />
                      <span class="text-gray-600">发起时间：</span>
                      <span class="text-black font-bold">{{ order.startTime }}</span>
                    </span>
                    <span>
                      <i class="i-mdi-flag mr-1 text-blue-400" />
                      <span class="text-gray-600">参考优先级：</span>
                      <span class="text-black font-bold">{{ order.referencePriority }}</span>
                    </span>
                    <span>
                      <i class="i-mdi-alert mr-1 text-blue-400" />
                      <span class="text-gray-600">任务优先级：</span>
                      <span class="text-black font-bold">{{ order.taskPriority }}</span>
                    </span>
                    <span>
                      <i class="i-mdi-progress-clock mr-1 text-blue-400" />
                      <span class="text-gray-600">当前状态：</span>
                      <span
                        class="ml-2 rounded-full px-3 py-1 font-bold"
                        :class="{
                          'bg-yellow-100 text-yellow-700': order.status === '草稿',
                          'bg-orange-100 text-orange-700': order.status === '待审批',
                          'bg-purple-100 text-purple-700': order.status === '待分发',
                          'bg-blue-100 text-blue-700': order.status === '进行中',
                          'bg-green-100 text-green-700': order.status === '已完成',
                          'bg-red-100 text-red-700': order.status === '已退回',
                        }"
                      >
                        {{ order.status }}
                      </span>
                    </span>
                  </div>
                </div>
              </div>
            </template>
            <div class="mt-4 space-y-3">
              <!-- 完成工单 -->
              <FaPageMain
                v-if="['进行中'].includes(order.status)"
                title=""
                :collaspe="!expandedMap[order.orderID]"
                height="auto"
                class="w-full"
              >
                <template #title>
                  <div class="w-full flex items-center justify-between">
                    <div class="flex items-center">
                      <span class="text-lg text-blue-900 font-bold">完成工单</span>
                      <span
                        v-if="order.status === '进行中'"
                        class="ml-2 inline-block align-middle"
                        style="width: 12px;height: 12px;background: #f59e42;border-radius: 50%;"
                        title="进行中"
                      />
                      <span
                        v-else-if="order.status === '已完成'"
                        class="ml-2 inline-block align-middle"
                        style="width: 12px;height: 12px;background: #22c55e;border-radius: 50%;"
                        title="已完成"
                      />
                    </div>
                    <div class="flex items-center gap-4 text-sm text-gray-700 font-bold">
                      <span>负责人：{{ order.executorID }}</span>
                      <span>完成时间：{{ order.finishTime }}</span>
                    </div>
                  </div>
                </template>
                <div
                  class="grid grid-cols-2 items-start gap-x-8 gap-y-4 rounded bg-gray-50 px-6 py-4"
                  :class="order.status === '进行中' ? 'bg-green-50' : 'bg-gray-100 opacity-70'"
                >
                  <!-- 问题复现类工单 -->
                  <template v-if="order.type === '问题复现'">
                    <div class="col-span-1 w-full flex items-center gap-2">
                      <span class="w-32 text-black font-semibold">
                        <span class="mr-1 text-red-500">*</span>
                        复现现象：</span>
                      <textarea
                        v-model="order.finishPhenomenon"
                        class="flex-1 resize-none border border-gray-200 rounded bg-white px-3 py-2 text-sm text-black"
                        rows="2"
                        :readonly="order.status === '已完成'"
                      />
                    </div>
                    <div class="col-span-1 w-full flex items-center gap-2">
                      <span class="w-32 text-black font-semibold">备注：</span>
                      <textarea
                        v-model="order.finishRemark"
                        class="flex-1 resize-none border border-gray-200 rounded bg-white px-3 py-2 text-sm text-black"
                        rows="2"
                        :readonly="order.status === '已完成'"
                      />
                    </div>
                  </template>

                  <!-- 版本迭代类工单 -->
                  <template v-else-if="order.type === '版本迭代'">
                    <div class="col-span-1 w-full flex items-center gap-2">
                      <span class="w-40 text-black font-semibold">
                        <span class="mr-1 text-red-500">*</span>
                        升级后模型版本：</span>
                      <div class="flex flex-1 flex-col gap-2">
                        <!-- 验证码样式的版本输入 -->
                        <div v-if="order.status !== '已完成'" class="version-input-container">
                          <!-- 显示创建时的完成版本作为基础 -->
                          <template v-if="order.completeModelVersion">
                            <template v-if="parseCompleteModelVersion(order.completeModelVersion || '').first && parseCompleteModelVersion(order.completeModelVersion || '').second && parseCompleteModelVersion(order.completeModelVersion || '').third">
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
                            <el-input
                              v-model="order.finishModelVersionNumber"
                              placeholder="0"
                              maxlength="5"
                              class="version-input"
                              @input="value => handleFinishVersionInput(order, 'number', value)"
                            />
                            <!-- 字母输入框 -->
                            <el-input
                              v-model="order.finishModelVersionLetter"
                              placeholder="A"
                              maxlength="1"
                              class="version-input"
                              @input="value => handleFinishVersionInput(order, 'letter', value)"
                            />
                          </template>

                          <!-- 如果没有completeModelVersion，显示传统输入框 -->
                          <template v-else>
                            <input
                              v-model="order.finishModelVersion"
                              class="border border-gray-200 rounded bg-white px-3 py-2 text-sm text-black"
                              :placeholder="`当前版本: ${order.modelVersionID}`"
                            >
                          </template>
                        </div>

                        <!-- 已完成状态：只读显示 -->
                        <div v-else class="version-input-container">
                          <template v-if="order.finishModelVersion">
                            <template v-for="(char, _index) in order.finishModelVersion.split('')" :key="index">
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
                          <div
                            v-if="expandedMap[order.orderID]"
                            class="mt-1 text-blue-600"
                          >
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
                    <div class="col-span-2 w-full flex items-center gap-2">
                      <span class="w-32 text-black font-semibold">备注：</span>
                      <textarea
                        v-model="order.finishRemark"
                        class="flex-1 resize-none border border-gray-200 rounded bg-white px-3 py-2 text-sm text-black"
                        rows="2"
                        :readonly="order.status === '已完成'"
                      />
                    </div>
                  </template>

                  <!-- 交付发送类工单 -->
                  <template v-else-if="order.type === '交付发送'">
                    <!-- 第一行：是否加密 -->
                    <div class="col-span-2 w-full flex items-center gap-2">
                      <span class="w-32 text-black font-semibold">
                        <span class="mr-1 text-red-500">*</span>
                        是否加密：</span>
                      <el-select
                        v-model="order.isEncrypted"
                        placeholder="请选择"
                        class="flex-1"
                        :disabled="order.status === '已完成'"
                      >
                        <el-option label="是" value="是" />
                        <el-option label="否" value="否" />
                      </el-select>
                    </div>

                    <!-- 第二行：外壳号和授权ID（只有选择加密时才显示） -->
                    <template v-if="order.isEncrypted === '是'">
                      <div class="col-span-1 w-full flex items-center gap-2">
                        <span class="w-32 text-black font-semibold">
                          <span class="mr-1 text-red-500">*</span>
                          外壳号：</span>
                        <el-select
                          v-model="order.finishShellNo"
                          placeholder="请选择或输入外壳号"
                          filterable
                          allow-create
                          default-first-option
                          :reserve-keyword="false"
                          class="flex-1"
                          :disabled="order.status === '已完成'"
                          @visible-change="val => val && fetchShellNumbers(order)"
                        >
                          <el-option
                            v-for="item in shellNumbers"
                            :key="item"
                            :label="item"
                            :value="item"
                          />
                        </el-select>
                      </div>
                      <!-- 修改授权ID下拉框部分 -->
                      <div class="col-span-1 w-full flex items-center gap-2">
                        <span class="w-32 text-black font-semibold">
                          <span class="mr-1 text-red-500">*</span>
                          授权ID：</span>
                        <div class="flex flex-1 items-center gap-2">
                          <!-- 显示选中的授权ID -->
                          <el-input
                            v-model="order.finishAuthId"
                            placeholder="请选择授权ID"
                            readonly
                            class="flex-1"
                            :disabled="order.status === '已完成'"
                          />
                          <!-- 选择按钮 -->
                          <el-button
                            type="primary"
                            size="small"
                            :disabled="order.status === '已完成'"
                            @click="handleAuthIdSelectClick(order)"
                          >
                            选择
                          </el-button>
                        </div>
                      </div>
                    </template>

                    <!-- 第三行：备注 -->
                    <div class="col-span-2 w-full flex items-center gap-2">
                      <span class="w-32 text-black font-semibold">备注：</span>
                      <textarea
                        v-model="order.finishRemark"
                        class="flex-1 resize-none border border-gray-200 rounded bg-white px-3 py-2 text-sm text-black"
                        rows="2"
                        :readonly="order.status === '已完成'"
                      />
                    </div>
                  </template>

                  <!-- 版本迭代+交付发送类工单 -->
                  <template v-else-if="order.type === '版本迭代+交付发送'">
                    <!-- 第一行：升级后模型版本和是否加密 -->
                    <div class="col-span-1 w-full flex items-center gap-2">
                      <span class="w-40 text-black font-semibold">
                        <span class="mr-1 text-red-500">*</span>
                        升级后模型版本：</span>
                      <div class="flex flex-1 flex-col gap-2">
                        <!-- 验证码样式的版本输入 -->
                        <div v-if="order.status !== '已完成'" class="version-input-container">
                          <!-- 显示创建时的完成版本作为基础 -->
                          <template v-if="order.completeModelVersion">
                            <template v-if="parseCompleteModelVersion(order.completeModelVersion || '').first && parseCompleteModelVersion(order.completeModelVersion || '').second && parseCompleteModelVersion(order.completeModelVersion || '').third">
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
                            <el-input
                              v-model="order.finishModelVersionNumber"
                              placeholder="0"
                              maxlength="5"
                              class="version-input"
                              @input="value => handleFinishVersionInput(order, 'number', value)"
                            />
                            <!-- 字母输入框 -->
                            <el-input
                              v-model="order.finishModelVersionLetter"
                              placeholder="A"
                              maxlength="1"
                              class="version-input"
                              @input="value => handleFinishVersionInput(order, 'letter', value)"
                            />
                          </template>

                          <!-- 如果没有completeModelVersion，显示传统输入框 -->
                          <template v-else>
                            <input
                              v-model="order.finishModelVersion"
                              class="border border-gray-200 rounded bg-white px-3 py-2 text-sm text-black"
                              :placeholder="`当前版本: ${order.modelVersionID}`"
                            >
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
                          <div
                            v-if="expandedMap[order.orderID]"
                            class="mt-1 text-blue-600"
                          >
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
                    <div class="col-span-1 w-full flex items-center gap-2">
                      <span class="w-32 text-black font-semibold">
                        <span class="mr-1 text-red-500">*</span>
                        是否加密：</span>
                      <el-select
                        v-model="order.isEncrypted"
                        placeholder="请选择"
                        class="flex-1"
                        :disabled="order.status === '已完成'"
                      >
                        <el-option label="是" value="是" />
                        <el-option label="否" value="否" />
                      </el-select>
                    </div>

                    <!-- 第二行：外壳号和授权ID（只有选择加密时才显示） -->
                    <template v-if="order.isEncrypted === '是'">
                      <div class="col-span-1 w-full flex items-center gap-2">
                        <span class="w-32 text-black font-semibold">
                          <span class="mr-1 text-red-500">*</span>
                          外壳号：</span>
                        <el-select
                          v-model="order.finishShellNo"
                          placeholder="请选择或输入外壳号"
                          filterable
                          allow-create
                          default-first-option
                          :reserve-keyword="false"
                          class="flex-1"
                          :disabled="order.status === '已完成'"
                          @visible-change="val => val && fetchShellNumbers(order)"
                        >
                          <el-option
                            v-for="item in shellNumbers"
                            :key="item"
                            :label="item"
                            :value="item"
                          />
                        </el-select>
                      </div>
                      <!-- 修改授权ID部分 -->
                      <div class="col-span-1 w-full flex items-center gap-2">
                        <span class="w-32 text-black font-semibold">
                          <span class="mr-1 text-red-500">*</span>
                          授权ID：</span>
                        <div class="flex flex-1 items-center gap-2">
                          <!-- 显示选中的授权ID -->
                          <el-input
                            v-model="order.finishAuthId"
                            placeholder="请选择授权ID"
                            readonly
                            class="flex-1"
                            :disabled="order.status === '已完成'"
                          />
                          <!-- 选择按钮 -->
                          <el-button
                            type="primary"
                            size="small"
                            :disabled="order.status === '已完成' || !order.finishShellNo"
                            @click="handleAuthIdSelectClick(order)"
                          >
                            选择
                          </el-button>
                        </div>
                      </div>
                    </template>

                    <!-- 第三行：备注 -->
                    <div class="col-span-2 w-full flex items-center gap-2">
                      <span class="w-32 text-black font-semibold">备注：</span>
                      <textarea
                        v-model="order.finishRemark"
                        class="flex-1 resize-none border border-gray-200 rounded bg-white px-3 py-2 text-sm text-black"
                        rows="2"
                        :readonly="order.status === '已完成'"
                      />
                    </div>
                  </template>

                  <!-- 功能开发类工单 -->
                  <template v-else-if="order.type === '功能开发'">
                    <div class="col-span-2 w-full flex items-center gap-2">
                      <span class="w-40 text-black font-semibold">
                        <span class="mr-1 text-red-500">*</span>
                        升级后模型版本：
                      </span>
                      <div class="flex flex-1 flex-col gap-2">
                        <!-- 验证码样式的版本输入 -->
                        <div v-if="order.status !== '已完成'" class="version-input-container">
                          <!-- 显示创建时的完成版本作为基础 -->
                          <template v-if="order.completeModelVersion">
                            <template v-if="parseCompleteModelVersion(order.completeModelVersion || '').first && parseCompleteModelVersion(order.completeModelVersion || '').second && parseCompleteModelVersion(order.completeModelVersion || '').third">
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
                            <el-input
                              v-model="order.finishModelVersionNumber"
                              placeholder="0"
                              maxlength="5"
                              class="version-input"
                              @input="value => handleFinishVersionInput(order, 'number', value)"
                            />
                            <!-- 字母输入框 -->
                            <el-input
                              v-model="order.finishModelVersionLetter"
                              placeholder="A"
                              maxlength="1"
                              class="version-input"
                              @input="value => handleFinishVersionInput(order, 'letter', value)"
                            />
                          </template>

                          <!-- 如果没有completeModelVersion，显示传统输入框 -->
                          <template v-else>
                            <input
                              v-model="order.finishModelVersionId"
                              class="border border-gray-200 rounded bg-white px-3 py-2 text-sm text-black"
                              :placeholder="`当前版本: ${order.modelVersionID}`"
                            >
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
                          <div
                            v-if="expandedMap[order.orderID]"
                            class="mt-1 text-blue-600"
                          >
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

                    <div class="col-span-2 w-full flex items-center gap-2">
                      <span class="w-40 text-black font-semibold">完成功能描述：</span>
                      <textarea
                        v-model="order.finishFeatureDesc"
                        class="flex-1 resize-none border border-gray-200 rounded bg-white px-3 py-2 text-sm text-black"
                        rows="2"
                        :readonly="order.status === '已完成'"
                      />
                    </div>
                  </template>

                  <!-- 其他类工单 -->
                  <template v-else-if="order.type === '其他'">
                    <div class="col-span-2 w-full flex items-center gap-2">
                      <span class="w-32 text-black font-semibold">
                        <span class="mr-1 text-red-500">*</span>
                        备注：
                      </span>
                      <textarea
                        v-model="order.finishRemarkOther"
                        class="flex-1 resize-none border border-gray-200 rounded bg-white px-3 py-2 text-sm text-black"
                        rows="2"
                        :readonly="order.status === '已完成'"
                      />
                    </div>
                  </template>
                  <!-- 修改完成工单内容块，按钮与流转信息分三行（流转按钮单独一行） -->
                  <div class="col-span-2 mt-4 flex flex-col items-center gap-4">
                    <!-- 第一行：提交完成工单按钮 -->
                    <el-button
                      type="primary"
                      size="large"
                      :disabled="order.status === '已完成' || !isFinishOrderFilled(order)"
                      @click="handleFinishOrderClick(order)"
                    >
                      提交完成工单
                    </el-button>

                    <!-- 第二行：分割线 -->
                    <hr class="my-4 w-full border-t-2 border-gray-300">
                    <!-- 第三行：流转内容区（直接绑定到 order） -->
                    <div class="w-full flex items-center gap-2">
                      <span class="w-32 text-black font-semibold">流转负责人：</span>
                      <el-select
                        v-model="order.transferExecutorIDEdit"
                        placeholder="请选择流转负责人ID"
                        filterable
                        clearable
                        size="small"
                        style="width: 140px;"
                        :disabled="order.status === '已完成'"
                        @visible-change="val => val && fetchTransferExecutorList(order.modelID)"
                      >
                        <el-option
                          v-for="item in transferExecutor"
                          :key="item.id"
                          :label="`${item.name} (${item.id})`"
                          :value="item.id"
                        />
                      </el-select>
                      <span class="ml-4 w-32 text-black font-semibold">
                        <span class="mr-1 text-red-500">*</span>
                        工作记录：</span>
                      <textarea
                        v-model="order.transferReasonEdit"
                        class="flex-1 resize-none border border-gray-200 rounded bg-white px-3 py-2 text-sm text-black"
                        rows="2"
                        :readonly="order.status === '已完成'"
                        style="min-width: 320px;"
                        placeholder="请输入工作记录"
                      />
                    </div>
                    <div class="w-full flex items-center justify-center">
                      <el-button
                        type="primary"
                        size="large"
                        :disabled="order.status === '已完成' || !order.transferExecutorIDEdit || !order.transferReasonEdit"
                        @click="handleTransferOrder(order)"
                      >
                        提交流转
                      </el-button>
                    </div>
                  </div>
                </div>
              </FaPageMain>

              <!-- 多次流转内容块，循环显示每一次流转（紧跟在任务分发后面），倒序显示 -->
              <template v-if="order.status === '进行中'">
                <template v-for="(transfer, idx) in (order.transfers ? [...order.transfers].reverse() : [])" :key="idx">
                  <FaPageMain
                    title=""
                    :collaspe="!expandedMap[order.orderID]"
                    height="auto"
                    class="w-full"
                  >
                    <template #title>
                      <div class="w-full flex items-center justify-between">
                        <div class="flex items-center">
                          <span class="text-lg text-blue-900 font-bold">
                            工单流转
                            {{ Array.isArray(order.transfers) && order.transfers.length > 1 ? `（第${order.transfers.length - idx}次）` : '' }}
                          </span>
                          <span
                            class="ml-2 inline-block align-middle"
                            style="width: 12px;height: 12px;background: #22c55e ;border-radius: 50%;"
                            title="流转"
                          />
                        </div>
                        <div class="flex items-center gap-4 text-sm text-gray-700 font-bold">
                          <span>流转负责人：{{ transfer.transferExecutorID }}</span>
                          <span>流转时间：{{ transfer.transferTime }}</span>
                        </div>
                      </div>
                    </template>
                    <div class="grid grid-cols-2 items-start gap-x-8 gap-y-4 rounded bg-gray-50 px-6 py-4">
                      <div class="col-span-2 w-full flex items-center gap-2">
                        <span class="w-32 text-black font-semibold">工作记录：</span>
                        <textarea
                          class="flex-1 resize-none border border-gray-200 rounded bg-gray-50 px-3 py-2 text-sm text-black"
                          :value="transfer.transferReason"
                          rows="2"
                          readonly
                        />
                      </div>
                    </div>
                  </FaPageMain>
                </template>
              </template>

              <!-- 任务分发 -->
              <FaPageMain
                v-if="['待分发', '进行中'].includes(order.status)"
                title=""
                :collaspe="!expandedMap[order.orderID]"
                height="auto"
                class="w-full"
              >
                <template #title>
                  <div class="w-full flex items-center justify-between pr-4">
                    <div class="flex items-center">
                      <span class="text-lg text-blue-900 font-bold">任务分发</span>
                      <!-- 只有"待分发"时橙点，"进行中"或"已完成"时绿点 -->
                      <span
                        v-if="order.status === '待分发'"
                        class="ml-2 inline-block align-middle"
                        style="width: 12px;height: 12px;background: #f59e42;border-radius: 50%;"
                        title="进行中"
                      />
                      <span
                        v-else-if="['进行中', '已完成'].includes(order.status)"
                        class="ml-2 inline-block align-middle"
                        style="width: 12px;height: 12px;background: #22c55e;border-radius: 50%;"
                        title="已完成"
                      />
                    </div>
                    <!-- 负责人和完成时间（分发人ID和分发时间） -->
                    <div class="flex items-center gap-4 text-sm text-gray-700 font-bold">
                      <span>负责人：{{ order.distributorID }}</span>
                      <span>完成时间：{{ order.distributeTime }}</span>
                    </div>
                  </div>
                </template>
                <div
                  class="flex items-center justify-between rounded px-6 py-4"
                  :class="order.status === '待分发' ? 'bg-gray-50' : 'bg-gray-100 opacity-70'"
                >
                  <div class="flex items-center gap-3">
                    <span class="text-gray-700 font-semibold">任务优先级：</span>
                    <!-- 修改这里：根据工单状态显示不同的组件 -->
                    <template v-if="order.status === '待分发'">
                      <!-- 待分发状态：显示下拉选择框，默认值为工单已有的任务优先级 -->
                      <el-select
                        v-model="leaderPriority"
                        placeholder="请选择"
                        size="small"
                        style="width: 110px;"
                      >
                        <el-option label="紧急" value="紧急" />
                        <el-option label="一般" value="一般" />
                      </el-select>
                    </template>
                    <template v-else>
                      <!-- 进行中状态：显示只读输入框，显示工单的任务优先级 -->
                      <el-input
                        :model-value="order.taskPriority"
                        size="small"
                        style="width: 110px;"
                        disabled
                      />
                    </template>

                    <!-- 新增：下一流程负责人（执行人ID） -->
                    <span class="ml-4 text-gray-700 font-semibold">下一流程负责人：</span>
                    <el-select
                      v-model="order.executorID"
                      placeholder="请选择执行人ID"
                      filterable
                      clearable
                      size="small"
                      style="width: 140px;"
                      :disabled="order.status !== '待分发'"
                      @visible-change="val => val && fetchExecutorList(order.modelID)"
                    >
                      <el-option
                        v-for="item in executor"
                        :key="item.id"
                        :label="`${item.name} (${item.id})`"
                        :value="item.id"
                      />
                    </el-select>
                  </div>
                  <div class="flex items-center gap-3">
                    <el-button
                      type="success"
                      size="small"
                      :disabled="order.status !== '待分发' || !leaderPriority"
                      @click="handleDistribute(order)"
                    >
                      <i class="i-mdi-check-circle-outline mr-1" /> 同意
                    </el-button>
                    <el-button
                      type="danger"
                      size="small"
                      :disabled="order.status !== '待分发'"
                      @click="handleReject(order)"
                    >
                      <i class="i-mdi-close-circle-outline mr-1" /> 拒绝
                    </el-button>
                  </div>
                </div>
              </FaPageMain>
              <!-- 任务审批 -->
              <FaPageMain
                v-if="['待审批', '待分发', '进行中'].includes(order.status)"
                title=""
                :collaspe="!expandedMap[order.orderID]"
                height="auto"
                class="w-full"
              >
                <template #title>
                  <div class="w-full flex items-center justify-between pr-4">
                    <div class="flex items-center">
                      <span class="text-lg text-blue-900 font-bold">任务审批</span>
                      <!-- 状态点 -->
                      <span
                        v-if="order.status === '待审批'"
                        class="ml-2 inline-block align-middle"
                        style="width: 12px;height: 12px;background: #f59e42;border-radius: 50%;"
                        title="进行中"
                      />
                      <span
                        v-else-if="['待分发', '进行中', '已完成'].includes(order.status)"
                        class="ml-2 inline-block align-middle"
                        style="width: 12px;height: 12px;background: #22c55e;border-radius: 50%;"
                        title="已完成"
                      />
                      <span
                        v-else-if="order.status === '已退回'"
                        class="ml-2 inline-block align-middle"
                        style="width: 12px;height: 12px;background: #ef4444;border-radius: 50%;"
                        title="已退回"
                      />
                    </div>
                    <!-- 负责人和完成时间（审批人ID和审批时间） -->
                    <div class="flex items-center gap-4 text-sm text-gray-700 font-bold">
                      <span>负责人：{{ order.approverID }}</span>
                      <span>完成时间：{{ order.approveTime }}</span>
                    </div>
                  </div>
                </template>
                <div
                  class="flex items-center justify-between rounded px-6 py-4"
                  :class="order.status === '待审批' ? 'bg-gray-50' : 'bg-gray-100 opacity-70'"
                >
                  <div class="flex items-center gap-3">
                    <span class="text-gray-700 font-semibold">参考优先级：</span>
                    <!-- 修改这里：根据工单状态显示不同的组件 -->
                    <template v-if="order.status === '待审批'">
                      <!-- 待审批状态：显示下拉选择框，默认值为工单已有的参考优先级 -->
                      <el-select
                        v-model="leaderPriority"
                        placeholder="请选择"
                        size="small"
                        style="width: 110px;"
                      >
                        <el-option label="紧急" value="紧急" />
                        <el-option label="一般" value="一般" />
                      </el-select>
                    </template>
                    <template v-else>
                      <!-- 非待审批状态：显示只读输入框，显示工单的参考优先级 -->
                      <el-input
                        :model-value="order.referencePriority"
                        size="small"
                        style="width: 110px;"
                        disabled
                      />
                    </template>

                    <!-- 下一流程负责人-分发人选择 -->
                    <span class="ml-4 text-gray-700 font-semibold">下一流程负责人：</span>
                    <el-select
                      v-model="order.distributorID"
                      placeholder="请选择负责人ID"
                      filterable
                      clearable
                      size="small"
                      style="width: 140px;"
                      :disabled="order.status !== '待审批'"
                      @visible-change="val => val && fetchDistributorList(order.modelID)"
                    >
                      <el-option
                        v-for="item in distributor"
                        :key="item.id"
                        :label="`${item.name} (${item.id})`"
                        :value="item.id"
                      />
                    </el-select>
                  </div>
                  <div class="flex items-center gap-3">
                    <el-button
                      type="success"
                      size="small"
                      :disabled="order.status !== '待审批' || !leaderPriority"
                      @click="handleApprove(order)"
                    >
                      <i class="i-mdi-check-circle-outline mr-1" /> 同意
                    </el-button>
                    <el-button
                      type="danger"
                      size="small"
                      :disabled="order.status !== '待审批'"
                      @click="handleReject(order)"
                    >
                      <i class="i-mdi-close-circle-outline mr-1" /> 拒绝
                    </el-button>
                  </div>
                </div>
              </FaPageMain>
              <!-- 任务发起（始终显示在最下方） -->
              <FaPageMain
                title=""
                :collaspe="!expandedMap[order.orderID]"
                height="auto"
                class="w-full"
              >
                <template #title>
                  <div class="w-full flex items-center justify-between pr-4">
                    <div class="flex items-center">
                      <span class="text-lg text-blue-900 font-bold">任务发起</span>
                      <span
                        class="ml-2 inline-block align-middle"
                        style="width: 12px;height: 12px;background: #22c55e;border-radius: 50%;"
                        title="已完成"
                      />
                      <!-- 标签和内容分开显示，标签小且不加粗，内容正常 -->
                      <span class="ml-3 text-sm text-gray-500">模型：</span>
                      <span class="ml-1 text-black font-semibold">{{ order.modelID }}</span>
                      <span class="ml-3 text-sm text-gray-500">基准版本：</span>
                      <span class="ml-1 text-black font-semibold">{{ order.modelVersionID }}</span>
                      <span
                        v-if="order.completeModelVersion"
                        class="ml-3 text-sm text-gray-500"
                      >
                        Matlab版本号：
                      </span>
                      <span
                        v-if="order.completeModelVersion"
                        class="ml-1 text-black font-semibold"
                      >
                        {{ order.completeModelVersion }}
                      </span>
                    </div>
                    <div class="flex items-center gap-4 text-sm text-gray-700 font-bold">
                      <span>负责人：{{ order.promoterID }}</span>
                      <span>完成时间：{{ order.startTime }}</span>
                    </div>
                  </div>
                </template>
                <div class="grid grid-cols-2 items-start gap-x-8 gap-y-4 rounded bg-gray-50 px-6 py-4">
                  <!-- 基础信息 -->
                  <div class="flex items-center gap-2">
                    <span class="w-32 text-black font-semibold">工单ID：</span>
                    <input class="flex-1 border border-gray-200 rounded bg-gray-50 px-3 py-2 text-blue-700 font-bold" :value="order.orderID" readonly>
                  </div>
                  <div class="flex items-center gap-2">
                    <span class="w-32 text-black font-semibold">工单类型：</span>
                    <input class="flex-1 border border-gray-200 rounded bg-gray-50 px-3 py-2 text-blue-700 font-bold" :value="order.type" readonly>
                  </div>
                  <div class="flex items-center gap-2">
                    <span class="w-32 text-black font-semibold">发起人ID：</span>
                    <input class="flex-1 border border-gray-200 rounded bg-gray-50 px-3 py-2 text-blue-700 font-bold" :value="order.promoterID" readonly>
                  </div>
                  <div class="flex items-center gap-2">
                    <span class="w-32 text-black font-semibold">发起时间：</span>
                    <input class="flex-1 border border-gray-200 rounded bg-gray-50 px-3 py-2 text-blue-700 font-bold" :value="order.startTime" readonly>
                  </div>

                  <!-- 问题复现类 -->
                  <template v-if="order.type === '问题复现'">
                    <div class="flex items-center gap-2">
                      <span class="w-32 text-black font-semibold">对应协调单：</span>
                      <input class="flex-1 border border-gray-200 rounded bg-gray-50 px-3 py-2 text-sm text-black" :value="order.coordinationID || 'NA'" readonly>
                    </div>
                    <div class="col-span-2 flex items-start gap-2">
                      <span class="w-32 text-black font-semibold">复现内容：</span>
                      <textarea class="flex-1 resize-none border border-gray-200 rounded bg-gray-50 px-3 py-2 text-sm text-black" :value="order.description" rows="2" readonly />
                    </div>
                    <div class="flex items-center gap-2">
                      <span class="w-32 text-black font-semibold">复现参考文件：</span>
                      <template v-if="order.files && order.files.length">
                        <div class="flex flex-wrap gap-2">
                          <span
                            v-for="(file, idx) in order.files"
                            :key="idx"
                            class="flex cursor-pointer items-center text-blue-600 underline hover:text-blue-800"
                            @click="downloadFile(file.fileUrl, file.fileName)"
                          >
                            <i class="i-mdi-download mr-1" />
                            {{ file.fileName }}
                          </span>
                        </div>
                      </template>
                      <span v-else class="text-gray-400">无</span>
                    </div>
                    <div class="flex items-center gap-2">
                      <span class="w-32 text-black font-semibold">审批人ID：</span>
                      <input class="flex-1 border border-gray-200 rounded bg-gray-50 px-3 py-2 text-sm text-black" :value="order.approverID || 'NA'" readonly>
                    </div>
                  </template>

                  <!-- 版本迭代类 -->
                  <template v-else-if="order.type === '版本迭代'">
                    <div class="flex items-center gap-2">
                      <span class="w-32 text-black font-semibold">升级后模型版本：</span>
                      <input class="flex-1 border border-gray-200 rounded bg-gray-50 px-3 py-2 text-sm text-black" :value="order.completeModelVersion" readonly>
                    </div>
                    <div class="flex items-center gap-2">
                      <span class="w-32 text-black font-semibold">对应协调单：</span>
                      <input class="flex-1 border border-gray-200 rounded bg-gray-50 px-3 py-2 text-sm text-black" :value="order.coordinationID || 'NA'" readonly>
                    </div>
                    <div class="flex items-center gap-2">
                      <span class="w-32 text-black font-semibold">更新内容：</span>
                      <textarea class="flex-1 resize-none border border-gray-200 rounded bg-gray-50 px-3 py-2 text-sm text-black" :value="order.updateNotes" rows="2" readonly />
                    </div>
                    <div class="flex items-center gap-2">
                      <span class="w-32 text-black font-semibold">封装要求：</span>
                      <textarea class="flex-1 resize-none border border-gray-200 rounded bg-gray-50 px-3 py-2 text-sm text-black" :value="order.packageRequirement" rows="2" readonly />
                    </div>
                    <div class="flex items-center gap-2">
                      <span class="w-32 text-black font-semibold">接口是否变化：</span>
                      <input class="flex-1 border border-gray-200 rounded bg-gray-50 px-3 py-2 text-sm text-black" :value="order.apiChanged" readonly>
                    </div>
                    <div class="flex items-center gap-2">
                      <span class="w-32 text-black font-semibold">审批人ID：</span>
                      <input class="flex-1 border border-gray-200 rounded bg-gray-50 px-3 py-2 text-sm text-black" :value="order.approverID || 'NA'" readonly>
                    </div>
                  </template>

                  <!-- 交付发送类 -->
                  <template v-else-if="order.type === '交付发送'">
                    <div class="flex items-center gap-2">
                      <span class="w-32 text-black font-semibold">目标客户：</span>
                      <input class="flex-1 border border-gray-200 rounded bg-gray-50 px-3 py-2 text-sm text-black" :value="order.targetCustomer" readonly>
                    </div>
                    <div class="flex items-center gap-2">
                      <span class="w-32 text-black font-semibold">CAE平台验证：</span>
                      <input class="flex-1 border border-gray-200 rounded bg-gray-50 px-3 py-2 text-sm text-black" :value="order.isCAEChecked" readonly>
                    </div>
                    <div class="flex items-center gap-2">
                      <span class="w-32 text-black font-semibold">包含敏感信息：</span>
                      <input class="flex-1 border border-gray-200 rounded bg-gray-50 px-3 py-2 text-sm text-black" :value="order.hasSensitiveInfo" readonly>
                    </div>
                    <div class="flex items-center gap-2">
                      <span class="w-32 text-black font-semibold">审批人ID：</span>
                      <input class="flex-1 border border-gray-200 rounded bg-gray-50 px-3 py-2 text-sm text-black" :value="order.approverID || 'NA'" readonly>
                    </div>
                  </template>

                  <!-- 版本迭代+交付发送类 -->
                  <template v-else-if="order.type === '版本迭代+交付发送'">
                    <div class="flex items-center gap-2">
                      <span class="w-32 text-black font-semibold">升级后模型版本：</span>
                      <input class="flex-1 border border-gray-200 rounded bg-gray-50 px-3 py-2 text-sm text-black" :value="order.completeModelVersion" readonly>
                    </div>
                    <div class="flex items-center gap-2">
                      <span class="w-32 text-black font-semibold">对应协调单：</span>
                      <input class="flex-1 border border-gray-200 rounded bg-gray-50 px-3 py-2 text-sm text-black" :value="order.coordinationID || 'NA'" readonly>
                    </div>
                    <div class="flex items-center gap-2">
                      <span class="w-32 text-black font-semibold">更新内容：</span>
                      <textarea class="flex-1 resize-none border border-gray-200 rounded bg-gray-50 px-3 py-2 text-sm text-black" :value="order.updateNotes" rows="2" readonly />
                    </div>
                    <div class="flex items-center gap-2">
                      <span class="w-32 text-black font-semibold">封装要求：</span>
                      <textarea class="flex-1 resize-none border border-gray-200 rounded bg-gray-50 px-3 py-2 text-sm text-black" :value="order.packageRequirement" rows="2" readonly />
                    </div>
                    <div class="flex items-center gap-2">
                      <span class="w-32 text-black font-semibold">接口是否变化：</span>
                      <input class="flex-1 border border-gray-200 rounded bg-gray-50 px-3 py-2 text-sm text-black" :value="order.apiChanged" readonly>
                    </div>
                    <div class="flex items-center gap-2">
                      <span class="w-32 text-black font-semibold">目标客户：</span>
                      <input class="flex-1 border border-gray-200 rounded bg-gray-50 px-3 py-2 text-sm text-black" :value="order.targetCustomer" readonly>
                    </div>
                    <div class="flex items-center gap-2">
                      <span class="w-32 text-black font-semibold">CAE平台验证：</span>
                      <input class="flex-1 border border-gray-200 rounded bg-gray-50 px-3 py-2 text-sm text-black" :value="order.isCAEChecked" readonly>
                    </div>
                    <div class="flex items-center gap-2">
                      <span class="w-32 text-black font-semibold">包含敏感信息：</span>
                      <input class="flex-1 border border-gray-200 rounded bg-gray-50 px-3 py-2 text-sm text-black" :value="order.hasSensitiveInfo" readonly>
                    </div>
                    <div class="flex items-center gap-2">
                      <span class="w-32 text-black font-semibold">审批人ID：</span>
                      <input class="flex-1 border border-gray-200 rounded bg-gray-50 px-3 py-2 text-sm text-black" :value="order.approverID || 'NA'" readonly>
                    </div>
                  </template>

                  <!-- 功能开发类 -->
                  <template v-else-if="order.type === '功能开发'">
                    <!-- <div class="flex items-center gap-2">
                      <span class="w-32 text-black font-semibold">升级后模型版本：</span>
                      <input class="flex-1 border border-gray-200 rounded bg-gray-50 px-3 py-2 text-sm text-black" :value="order.completeModelVersion" readonly>
                    </div> -->
                    <div class="flex items-start gap-2">
                      <span class="w-32 text-black font-semibold">功能描述：</span>
                      <textarea class="flex-1 resize-none border border-gray-200 rounded bg-gray-50 px-3 py-2 text-sm text-black" :value="order.featureDesc" rows="2" readonly />
                    </div>
                    <div class="flex items-center gap-2">
                      <span class="w-32 text-black font-semibold">审批人ID：</span>
                      <input class="flex-1 border border-gray-200 rounded bg-gray-50 px-3 py-2 text-sm text-black" :value="order.approverID || 'NA'" readonly>
                    </div>
                  </template>

                  <!-- 其他类 -->
                  <template v-else-if="order.type === '其他'">
                    <!-- <div class="flex items-center gap-2">
                      <span class="w-32 text-black font-semibold">模型ID：</span>
                      <input class="flex-1 border border-gray-200 rounded bg-gray-50 px-3 py-2 text-sm text-black" :value="order.modelID" readonly>
                    </div>
                    <div class="flex items-center gap-2">
                      <span class="w-32 text-black font-semibold">模型版本ID：</span>
                      <input class="flex-1 border border-gray-200 rounded bg-gray-50 px-3 py-2 text-sm text-black" :value="order.modelVersionID" readonly>
                    </div> -->
                    <div class="flex items-start gap-2">
                      <span class="w-32 text-black font-semibold">内容描述：</span>
                      <textarea class="flex-1 resize-none border border-gray-200 rounded bg-gray-50 px-3 py-2 text-sm text-black" :value="order.contentDesc" rows="2" readonly />
                    </div>
                    <div class="flex items-center gap-2">
                      <span class="w-32 text-black font-semibold">审批人ID：</span>
                      <input class="flex-1 border border-gray-200 rounded bg-gray-50 px-3 py-2 text-sm text-black" :value="order.approverID || 'NA'" readonly>
                    </div>
                  </template>
                </div>
              </FaPageMain>
            </div>
            <div class="mt-6" />
          </FaPageMain>
          <!-- 拒绝原因弹窗 -->
          <el-dialog
            v-model="rejectDialogVisible"
            title="填写拒绝原因"
            width="400px"
            :close-on-click-modal="false"
          >
            <el-form>
              <el-form-item label="拒绝原因" required>
                <el-input
                  v-model="rejectReasonInput"
                  type="textarea"
                  :rows="3"
                  placeholder="请填写拒绝原因"
                  maxlength="200"
                  show-word-limit
                />
              </el-form-item>
            </el-form>
            <template #footer>
              <el-button @click="rejectDialogVisible = false">
                取消
              </el-button>
              <el-button
                type="primary"
                :disabled="!rejectReasonInput"
                @click="confirmReject"
              >
                确定
              </el-button>
            </template>
          </el-dialog>
          <!-- 批量审批弹窗 -->
          <el-dialog
            v-model="batchApproveDialogVisible"
            title="批量审批"
            width="400px"
            :close-on-click-modal="false"
          >
            <el-form>
              <el-form-item label="参考优先级" required>
                <el-select v-model="batchLeaderPriority" placeholder="请选择">
                  <el-option label="紧急" value="紧急" />
                  <el-option label="一般" value="一般" />
                </el-select>
              </el-form-item>
              <el-form-item label="下一流程负责人ID" required>
                <el-select
                  v-model="batchDistributorID"
                  placeholder="请选择分发人ID"
                  filterable
                  clearable
                  style="width: 100%;"
                  @visible-change="val => val && fetchDistributorList(getBatchModelId())"
                >
                  <el-option
                    v-for="item in distributor"
                    :key="item.id"
                    :label="`${item.name} (${item.id})`"
                    :value="item.id"
                  />
                </el-select>
              </el-form-item>
            </el-form>
            <template #footer>
              <el-button @click="batchApproveDialogVisible = false">
                取消
              </el-button>
              <el-button
                type="primary"
                :disabled="!batchLeaderPriority || !batchDistributorID"
                @click="confirmBatchApprove"
              >
                确定
              </el-button>
            </template>
          </el-dialog>
          <!-- 批量分发弹窗 -->
          <el-dialog
            v-model="batchDistributeDialogVisible"
            title="批量分发"
            width="400px"
            :close-on-click-modal="false"
          >
            <el-form>
              <el-form-item label="任务优先级" required>
                <el-select v-model="batchTaskPriority" placeholder="请选择">
                  <el-option label="紧急" value="紧急" />
                  <el-option label="一般" value="一般" />
                </el-select>
              </el-form-item>
              <el-form-item label="下一流程负责人ID" required>
                <el-select
                  v-model="batchExecutorID"
                  placeholder="请选择执行人ID"
                  filterable
                  clearable
                  style="width: 100%;"
                  @visible-change="val => val && fetchExecutorList(getBatchModelId())"
                >
                  <el-option
                    v-for="item in executor"
                    :key="item.id"
                    :label="`${item.name} (${item.id})`"
                    :value="item.id"
                  />
                </el-select>
              </el-form-item>
            </el-form>
            <template #footer>
              <el-button @click="batchDistributeDialogVisible = false">
                取消
              </el-button>
              <el-button
                type="primary"
                :disabled="!batchTaskPriority || !batchExecutorID"
                @click="confirmBatchDistribute"
              >
                确定
              </el-button>
            </template>
          </el-dialog>

          <!-- 完成工单确认弹窗 -->
          <el-dialog
            v-model="finishConfirmDialogVisible"
            :title="`确认提交完成工单 #${confirmOrder?.orderID}`"
            width="600px"
            :close-on-click-modal="false"
          >
            <div v-if="confirmOrder" class="space-y-4">
              <!-- 工单基本信息 -->
              <div class="rounded bg-gray-50 p-4">
                <h4 class="mb-3 text-lg text-gray-800 font-semibold">
                  工单基本信息
                </h4>
                <div class="grid grid-cols-2 gap-4 text-sm">
                  <div>
                    <span class="text-gray-600 font-medium">工单ID：</span>
                    <span class="text-blue-700 font-bold">{{ confirmOrder.orderID }}</span>
                  </div>
                  <div>
                    <span class="text-gray-600 font-medium">工单类型：</span>
                    <span class="text-blue-700 font-bold">{{ confirmOrder.type }}</span>
                  </div>
                  <div>
                    <span class="text-gray-600 font-medium">模型ID：</span>
                    <span class="text-blue-700 font-bold">{{ confirmOrder.modelID }}</span>
                  </div>
                  <div>
                    <span class="text-gray-600 font-medium">当前版本：</span>
                    <span class="text-blue-700 font-bold">{{ confirmOrder.modelVersionID }}</span>
                  </div>
                </div>
              </div>

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
                  <div class="text-sm space-y-3">
                    <div>
                      <span class="text-gray-600 font-medium">升级后模型版本：</span>
                      <div class="mt-1 border rounded bg-white p-2 text-blue-700 font-bold">
                        {{ confirmOrder.finishModelVersion || '未填写' }}
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

                <!-- 交付发送类工单 -->
                <template v-else-if="confirmOrder.type === '交付发送'">
                  <div class="text-sm space-y-3">
                    <div>
                      <span class="text-gray-600 font-medium">是否加密：</span>
                      <span class="ml-2 rounded px-2 py-1 text-white font-bold" :class="confirmOrder.isEncrypted === '是' ? 'bg-red-500' : 'bg-green-500'">
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
                      <span class="text-gray-600 font-medium">备注：</span>
                      <div class="mt-1 border rounded bg-white p-2">
                        {{ confirmOrder.finishRemark || '未填写' }}
                      </div>
                    </div>
                  </div>
                </template>

                <!-- 版本迭代+交付发送类工单 -->
                <template v-else-if="confirmOrder.type === '版本迭代+交付发送'">
                  <div class="text-sm space-y-3">
                    <div>
                      <span class="text-gray-600 font-medium">升级后模型版本：</span>
                      <div class="mt-1 border rounded bg-white p-2 text-blue-700 font-bold">
                        {{ confirmOrder.finishModelVersion || '未填写' }}
                      </div>
                    </div>
                    <div>
                      <span class="text-gray-600 font-medium">是否加密：</span>
                      <span class="ml-2 rounded px-2 py-1 text-white font-bold" :class="confirmOrder.isEncrypted === '是' ? 'bg-red-500' : 'bg-green-500'">
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
                      <span class="text-gray-600 font-medium">备注：</span>
                      <div class="mt-1 border rounded bg-white p-2">
                        {{ confirmOrder.finishRemark || '未填写' }}
                      </div>
                    </div>
                  </div>
                </template>

                <!-- 功能开发类工单 -->
                <template v-else-if="confirmOrder.type === '功能开发'">
                  <div class="text-sm space-y-3">
                    <div>
                      <span class="text-gray-600 font-medium">升级后模型版本：</span>
                      <div class="mt-1 border rounded bg-white p-2 text-blue-700 font-bold">
                        {{ confirmOrder.finishModelVersionId || '未填写' }}
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
                    <strong>提示：</strong>提交后工单状态将变更为"已完成"，请确认以上信息无误后再提交。
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

          <!-- 授权ID选择弹窗 -->
          <el-dialog
            v-model="authSelectDialogVisible"
            title="选择授权ID"
            width="600px"
            :close-on-click-modal="false"
            @close="handleCloseAuthSelect"
          >
            <template #header>
              <div class="w-full flex items-center justify-between">
                <div class="flex items-center gap-2">
                  <el-icon class="text-xl text-blue-600">
                    <Key />
                  </el-icon>
                  <span class="text-lg font-bold">选择授权ID</span>
                </div>
                <span v-if="currentOrder" class="text-base text-gray-600 font-bold">
                  客户：{{ currentOrder.targetCustomer }}
                </span>
              </div>
            </template>

            <div v-if="authDetailsForSelection.length > 0" class="space-y-3">
              <!-- 授权ID列表 -->
              <div
                v-for="authDetail in authDetailsForSelection"
                :key="authDetail.authId"
                class="cursor-pointer border border-gray-200 rounded-lg bg-white shadow-sm transition-shadow duration-200 hover:shadow-md"
                @click="selectAuthId(authDetail.authId)"
              >
                <div class="p-4">
                  <div class="grid grid-cols-3 items-center gap-4">
                    <!-- 授权ID -->
                    <div class="flex flex-col">
                      <span class="text-sm text-gray-600 font-medium">授权ID</span>
                      <span class="text-lg text-blue-700 font-bold">{{ authDetail.authId }}</span>
                    </div>

                    <!-- 剩余天数 -->
                    <div class="flex flex-col">
                      <span class="text-sm text-gray-600 font-medium">剩余天数</span>
                      <span
                        class="text-lg font-bold"
                        :class="authDetail.remainingDaysColor"
                      >
                        {{ authDetail.remainingDays }}
                      </span>
                    </div>

                    <!-- 设备类型 -->
                    <div class="flex flex-col">
                      <span class="text-sm text-gray-600 font-medium">设备类型</span>
                      <span class="text-lg text-blue-700 font-bold">{{ authDetail.deviceType }}</span>
                    </div>
                  </div>

                  <!-- 选择指示器 -->
                  <div class="mt-3 flex justify-end">
                    <span class="text-sm text-blue-600 hover:text-blue-800">
                      点击选择此授权ID
                    </span>
                  </div>
                </div>
              </div>
            </div>

            <!-- 空状态 -->
            <div v-else class="py-8 text-center text-gray-500">
              <el-icon class="mb-2 text-4xl">
                <DocumentDelete />
              </el-icon>
              <div>暂无可用的授权ID</div>
            </div>

            <template #footer>
              <div class="flex justify-end">
                <el-button @click="handleCloseAuthSelect">
                  取消
                </el-button>
              </div>
            </template>
          </el-dialog>
        </div>
      </template>
    </el-skeleton>
  </div>
</template>

<style scoped>
/* 版本输入容器 - 验证码样式 */
.version-input-container {
  display: flex;
  flex-wrap: wrap; /* 添加换行支持 */
  gap: 6px;
  align-items: center;
  justify-content: flex-start;
}

/* 版本部分样式 */
.version-part {
  display: flex;
  flex-shrink: 0; /* 防止收缩 */
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
  flex-shrink: 0; /* 防止收缩 */
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
@media (width <= 640px) {
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
