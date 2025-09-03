<route lang="yaml">
meta:
  title: 工单创建
</route>

<!-- 路由元信息，设置页面标题和图标 -->
<script setup lang="ts">
import { ElMessage } from 'element-plus'
import { ref } from 'vue'
import { useRoute } from 'vue-router'
import orderApi from '@/api/modules/order'
import { useClientSuffixStore } from '@/store/modules/clientSuffix'
import { useUserStore } from '@/store/modules/user'

// -----------------数据结构-----------------
// 弹窗控制
const dialogProblemVisible = ref(false)// 问题复现工单弹窗控制
const dialogIterVisible = ref(false)// 版本迭代工单弹窗控制
const dialogDeliverVisible = ref(false)// 交付发送工单弹窗控制
const dialogIterDeliverVisible = ref(false)// 版本迭代并交付发送工单弹窗控制
const dialogDevVisible = ref(false)// 功能开发工单弹窗控制
const dialogOtherVisible = ref(false)// 其他工单弹窗控制

const userStore = useUserStore()
const clientSuffixStore = useClientSuffixStore()
const currentUserId = userStore.account // 获取当前登录用户ID

const route = useRoute()

// -----------------问题复现工单-----------------
// 问题复现工单表单数据（上传后端）
interface ProblemOrderForm {
  modelId: string // 模型ID
  modelVersionID: string // 模型版本ID
  coordinationId: string // 协调单号
  description: string // 复现内容描述
  approverID: string // 审批人ID
  files: any[] // 附件列表
}
const problemOrderForm = ref<ProblemOrderForm>({
  modelId: '', // 模型ID
  modelVersionID: '', // 模型版本ID
  coordinationId: '', // 协调单号
  description: '', // 复现内容描述
  approverID: '', // 审批人ID
  files: [], // 附件列表
})

// 处理附件上传时的回调，更新表单中的文件列表
function handleProblemFileChange(file: any, fileList: any[]) {
  // 只保留最新上传的文件
  if (fileList.length > 1) {
    // 只保留最后一个文件，重新赋值为新数组
    problemOrderForm.value.files = [fileList[fileList.length - 1]]
  }
  else {
    problemOrderForm.value.files = [...fileList]
  }
}

// 提交问题复现工单的方法（使用FormData格式上传文件）
async function submitProblemOrder() {
  // 校验必填项
  if (
    !problemOrderForm.value.modelId
    || !problemOrderForm.value.modelVersionID
    || !problemOrderForm.value.description
    || !problemOrderForm.value.approverID
  ) {
    ElMessage.error('请完整填写所有必填项')
    return
  }
  // 构建FormData对象
  const formData = new FormData()
  formData.append('orderID', '')
  formData.append('status', '待审批')
  formData.append('promoterID', currentUserId)
  formData.append('modelID', problemOrderForm.value.modelId)
  formData.append('modelVersionID', problemOrderForm.value.modelVersionID)
  formData.append('coordinationID', problemOrderForm.value.coordinationId)
  formData.append('description', problemOrderForm.value.description)
  formData.append('approverID', problemOrderForm.value.approverID)
  formData.append('startTime', new Date().toISOString().slice(0, 19).replace('T', ' '))
  // 附件列表，逐个添加
  problemOrderForm.value.files.forEach((file) => {
    // file.raw 是 el-upload 组件的原始文件对象
    formData.append('files', file.raw)
  })
  // 提交到后端
  const res = await orderApi.submitProblemOrder(formData)
  // 显示后端返回的 message
  if (res?.data?.message) {
    ElMessage.success(res.data.message)
  }
  dialogProblemVisible.value = false
  // 清空表单
  problemOrderForm.value = {
    modelId: '',
    modelVersionID: '',
    coordinationId: '',
    description: '',
    files: [],
    approverID: '',
  }
}

// -----------------版本迭代工单-----------------
// 版本迭代工单表单（上传后端）
const iterOrderForm = ref({
  modelId: '', // 模型ID
  modelVersionID: '', // 模型版本ID
  completeModelVersionFirst: '', // 新增：第一位
  completeModelVersionSecond: '', // 新增：第二位
  completeModelVersionNumber: '', // 完成模型版本号（只输入数字部分）
  coordinationId: '', // 协调单号
  updateNotes: '', // 更新说明
  packageRequirement: '', // 封装要求
  apiChanged: '', // 接口是否变化
  approverID: '', // 审批人ID
})

// 初始化时自动填充前两位
watch(() => iterOrderForm.value.modelVersionID, (val) => {
  const parts = val ? val.split('.') : []
  iterOrderForm.value.completeModelVersionFirst = parts[0] || ''
  iterOrderForm.value.completeModelVersionSecond = parts[1] || ''
})

// 修改：计算版本前缀的显示
const versionParts = computed(() => {
  if (!iterOrderForm.value.modelVersionID) {
    return { first: '', second: '' }
  }
  // 直接分割，不处理 V
  const parts = iterOrderForm.value.modelVersionID.split('.')
  if (parts.length >= 2) {
    return {
      first: parts[0], // '4'
      second: parts[1], // '1'
    }
  }
  return { first: '', second: '' }
})

// 完整版本号拼接
const completeModelVersionDisplay = computed(() => {
  const first = iterOrderForm.value.completeModelVersionFirst || versionParts.value.first
  const second = iterOrderForm.value.completeModelVersionSecond || versionParts.value.second
  const third = iterOrderForm.value.completeModelVersionNumber
  if (first && second && third) {
    return `${first}.${second}.${third}`
  }
  return ''
})

// 新增：计算数字输入框的placeholder
const versionNumberPlaceholder = computed(() => {
  if (!iterOrderForm.value.modelVersionID) {
    return ''
  }
  const versionParts = iterOrderForm.value.modelVersionID.split('.')
  if (versionParts.length >= 3) {
    const thirdPart = versionParts[2]
    const numberMatch = thirdPart.match(/^\d+/)
    if (numberMatch) {
      const currentNumber = Number.parseInt(numberMatch[0])
      return String(currentNumber + 1)
    }
  }
  return '1'
})

// 新增：验证完成模型版本号格式（第三位允许大小写字母和数字）
function validateCompleteVersionNumber(value: string): boolean {
  // 只允许输入大小写字母和数字，至少1位
  return /^[a-z0-9]+$/i.test(value)
}

// 新增：处理完成模型版本号输入
function handleCompleteVersionInput(value: string) {
  // 只保留大小写字母和数字
  iterOrderForm.value.completeModelVersionNumber = value.replace(/[^a-z0-9]/gi, '')
}

// 提交版本迭代工单方法
// 修改：提交版本迭代工单方法
async function submitIterOrder() {
  // 校验必填项
  if (
    !iterOrderForm.value.modelId
    || !iterOrderForm.value.modelVersionID
    || !iterOrderForm.value.completeModelVersionNumber
    || !iterOrderForm.value.updateNotes
    || !iterOrderForm.value.apiChanged
    || !iterOrderForm.value.approverID
  ) {
    ElMessage.error('请完整填写所有必填项')
    return
  }

  // 验证完成模型版本号格式
  if (!validateCompleteVersionNumber(iterOrderForm.value.completeModelVersionNumber)) {
    ElMessage.error('完成模型版本号只能输入数字')
    return
  }

  // 提交到后端
  const res = await orderApi.submitIterOrder({
    orderID: '', // 新建时可为空或由后端生成
    status: '待审批', // 初始状态为待审批
    promoterID: String(currentUserId), // 当前用户ID
    modelID: iterOrderForm.value.modelId, // 模型ID
    modelVersionID: iterOrderForm.value.modelVersionID, // 模型版本ID
    completeModelVersion: completeModelVersionDisplay.value, // 完整的完成模型版本
    coordinationID: iterOrderForm.value.coordinationId, // 协调单号
    updateNotes: iterOrderForm.value.updateNotes, // 更新说明
    packageRequirement: iterOrderForm.value.packageRequirement, // 封装要求
    apiChanged: iterOrderForm.value.apiChanged, // 接口是否变化
    approverID: String(iterOrderForm.value.approverID), // 审批人ID
    startTime: new Date().toISOString().slice(0, 19).replace('T', ' '), // 开始时间(自动获取)
  })

  // 显示后端返回的 message
  if (res?.data?.message) {
    ElMessage.success(res.data.message)
  }

  dialogIterVisible.value = false
  // 清空表单
  iterOrderForm.value = {
    modelId: '',
    modelVersionID: '',
    completeModelVersionFirst: '',
    completeModelVersionSecond: '',
    completeModelVersionNumber: '',
    coordinationId: '',
    updateNotes: '',
    packageRequirement: '',
    apiChanged: '',
    approverID: '',
  }
}

// -----------------交付发送工单-----------------
// 交付发送工单表单（上传后端）
const deliverOrderForm = ref({
  modelId: '', // 模型ID
  modelVersionID: '', // 模型版本ID
  targetCustomer: '', // 目标客户
  isCAEChecked: '', // 是否CAE检查
  hasSensitiveInfo: '', // 是否包含敏感信息
  approverID: '', // 审批人ID
})

// 计算属性：获取交付发送工单选中客户的后缀列表
const deliverCustomerSuffixes = computed(() => {
  if (!deliverOrderForm.value.targetCustomer) {
    return []
  }
  return clientSuffixStore.getSuffixesByClient(deliverOrderForm.value.targetCustomer)
})

const deliverConfirmDialogVisible = ref(false)
const deliverConfirmInfo = ref<{ customer: string, suffix: string, isMatch: boolean }>({ customer: '', suffix: '', isMatch: false })

// 获取模型版本后缀（最后一位字母）
function getModelVersionSuffix(modelVersionID: string): string {
  // 取最后一段
  const lastPart = modelVersionID.split('.').pop() || ''
  // 只保留最后一位字母
  const match = lastPart.match(/[a-z]$/i)
  return match ? match[0] : ''
}

async function handleDeliverOrderSubmit() {
  // 校验必填项
  if (
    !deliverOrderForm.value.modelId
    || !deliverOrderForm.value.modelVersionID
    || !deliverOrderForm.value.targetCustomer
    || !deliverOrderForm.value.isCAEChecked
    || !deliverOrderForm.value.hasSensitiveInfo
    || !deliverOrderForm.value.approverID
  ) {
    ElMessage.error('请完整填写所有必填项')
    return
  }

  // 获取后缀和匹配结果
  const modelVersionSuffix = getModelVersionSuffix(deliverOrderForm.value.modelVersionID)
  const isMatch = clientSuffixStore.validateSuffixForClient(deliverOrderForm.value.targetCustomer, modelVersionSuffix)

  // 弹出二次确认弹窗，内容根据匹配结果显示
  deliverConfirmInfo.value = {
    customer: deliverOrderForm.value.targetCustomer,
    suffix: modelVersionSuffix,
    isMatch,
  }
  deliverConfirmDialogVisible.value = true
}

// 二次确认弹窗确认提交
async function confirmDeliverOrderSubmit() {
  deliverConfirmDialogVisible.value = false
  await submitDeliverOrder()
}

// 提交交付发送工单的方法
async function submitDeliverOrder() {
  // 校验必填项
  if (
    !deliverOrderForm.value.modelId
    || !deliverOrderForm.value.modelVersionID
    || !deliverOrderForm.value.targetCustomer
    || !deliverOrderForm.value.isCAEChecked
    || !deliverOrderForm.value.hasSensitiveInfo
    || !deliverOrderForm.value.approverID
  ) {
    ElMessage.error('请完整填写所有必填项')
    return
  }

  // 提交到后端
  const res = await orderApi.submitDeliverOrder({
    orderID: '', // 新建时可为空或由后端生成
    status: '待审批', // 初始状态为待审批
    promoterID: String(currentUserId), // 当前用户ID
    modelID: deliverOrderForm.value.modelId, // 模型ID
    modelVersionID: deliverOrderForm.value.modelVersionID, // 模型版本ID
    targetCustomer: deliverOrderForm.value.targetCustomer, // 目标客户
    isCAEChecked: deliverOrderForm.value.isCAEChecked, // 是否CAE检查
    hasSensitiveInfo: deliverOrderForm.value.hasSensitiveInfo, // 是否包含敏感信息
    approverID: String(deliverOrderForm.value.approverID), // 审批人ID
    startTime: new Date().toISOString().slice(0, 19).replace('T', ' '), // 开始时间(自动获取)
  })
  // 显示后端返回的 message
  if (res?.data?.message) {
    ElMessage.success(res.data.message)
  }

  dialogDeliverVisible.value = false
  // 清空表单
  deliverOrderForm.value = {
    modelId: '',
    modelVersionID: '',
    targetCustomer: '',
    isCAEChecked: '',
    hasSensitiveInfo: '',
    approverID: '',
  }
}

// -----------------模型迭代+交付发送工单-----------------
// 模型迭代+交付发送工单表单（上传后端）
const iterDeliverOrderForm = ref({
  modelId: '', // 模型ID
  modelVersionID: '', // 模型版本ID
  completeModelVersionFirst: '', // 新增：第一位
  completeModelVersionSecond: '', // 新增：第二位
  completeModelVersionNumber: '', // 新增：完成模型版本号（只输入数字部分）
  coordinationId: '', // 协调单号
  updateNotes: '', // 更新说明
  packageRequirement: '', // 封装要求
  apiChanged: '', // 接口是否变化
  targetCustomer: '', // 目标客户
  isCAEChecked: '', // 是否CAE检查
  hasSensitiveInfo: '', // 是否包含敏感信息
  approverID: '', // 审批人ID
})

// 计算属性：获取版本迭代+交付发送工单选中客户的后缀列表
const iterDeliverCustomerSuffixes = computed(() => {
  if (!iterDeliverOrderForm.value.targetCustomer) {
    return []
  }
  return clientSuffixStore.getSuffixesByClient(iterDeliverOrderForm.value.targetCustomer)
})

watch(() => iterDeliverOrderForm.value.modelVersionID, (val) => {
  const parts = val ? val.split('.') : []
  iterDeliverOrderForm.value.completeModelVersionFirst = parts[0] || ''
  iterDeliverOrderForm.value.completeModelVersionSecond = parts[1] || ''
})

// 版本前缀计算（去掉 substring(1)，直接用数字）
const iterDeliverVersionParts = computed(() => {
  if (!iterDeliverOrderForm.value.modelVersionID) {
    return { first: '', second: '' }
  }
  const parts = iterDeliverOrderForm.value.modelVersionID.split('.')
  if (parts.length >= 2) {
    return {
      first: parts[0], // 例如 '4'
      second: parts[1], // 例如 '1'
    }
  }
  return { first: '', second: '' }
})

// 版本迭代+交付发送工单的完成模型版本计算
const iterDeliverCompleteModelVersionDisplay = computed(() => {
  const first = iterDeliverOrderForm.value.completeModelVersionFirst || iterDeliverVersionParts.value.first
  const second = iterDeliverOrderForm.value.completeModelVersionSecond || iterDeliverVersionParts.value.second
  const third = iterDeliverOrderForm.value.completeModelVersionNumber
  if (first && second && third) {
    return `${first}.${second}.${third}`
  }
  return ''
})

// 数字输入框placeholder
const iterDeliverVersionNumberPlaceholder = computed(() => {
  if (!iterDeliverOrderForm.value.modelVersionID) {
    return ''
  }
  const versionParts = iterDeliverOrderForm.value.modelVersionID.split('.')
  if (versionParts.length >= 3) {
    const thirdPart = versionParts[2]
    const numberMatch = thirdPart.match(/^\d+/)
    if (numberMatch) {
      const currentNumber = Number.parseInt(numberMatch[0])
      return String(currentNumber + 1)
    }
  }
  return '1'
})

// 新增：版本迭代+交付发送工单的完成模型版本号输入处理
function handleIterDeliverCompleteVersionInput(value: string) {
  // 只保留大小写字母和数字
  iterDeliverOrderForm.value.completeModelVersionNumber = value.replace(/[^a-z0-9]/gi, '')
}

// 新增：版本迭代+交付发送工单二次确认弹窗控制变量
const iterDeliverConfirmDialogVisible = ref(false)
const iterDeliverConfirmInfo = ref<{ customer: string, suffix: string, isMatch: boolean }>({ customer: '', suffix: '', isMatch: false })

// 修改：版本迭代+交付发送工单提交方法，弹出二次确认弹窗
async function submitIterDeliverOrder() {
  // 校验必填项
  if (
    !iterDeliverOrderForm.value.modelId
    || !iterDeliverOrderForm.value.modelVersionID
    || !iterDeliverOrderForm.value.completeModelVersionNumber
    || !iterDeliverOrderForm.value.updateNotes
    || !iterDeliverOrderForm.value.apiChanged
    || !iterDeliverOrderForm.value.targetCustomer
    || !iterDeliverOrderForm.value.isCAEChecked
    || !iterDeliverOrderForm.value.hasSensitiveInfo
    || !iterDeliverOrderForm.value.approverID
  ) {
    ElMessage.error('请完整填写所有必填项')
    return
  }

  // 验证完成模型版本号格式
  if (!validateCompleteVersionNumber(iterDeliverOrderForm.value.completeModelVersionNumber)) {
    ElMessage.error('完成模型版本号只能输入数字或字母')
    return
  }

  // 获取后缀和匹配结果
  const modelVersionSuffix = getModelVersionSuffix(iterDeliverOrderForm.value.modelVersionID)
  const isMatch = clientSuffixStore.validateSuffixForClient(iterDeliverOrderForm.value.targetCustomer, modelVersionSuffix)

  // 弹出二次确认弹窗，内容根据匹配结果显示
  iterDeliverConfirmInfo.value = {
    customer: iterDeliverOrderForm.value.targetCustomer,
    suffix: modelVersionSuffix,
    isMatch,
  }
  iterDeliverConfirmDialogVisible.value = true
}

// 新增：版本迭代+交付发送工单二次确认弹窗确认提交
async function confirmIterDeliverOrderSubmit() {
  iterDeliverConfirmDialogVisible.value = false
  // 提交到后端
  const res = await orderApi.submitIterDeliverOrder({
    orderID: '', // 新建时可为空或由后端生成
    status: '待审批', // 初始状态为待审批
    promoterID: String(currentUserId), // 当前用户ID
    modelID: iterDeliverOrderForm.value.modelId, // 模型ID
    modelVersionID: iterDeliverOrderForm.value.modelVersionID, // 模型版本ID
    completeModelVersion: iterDeliverCompleteModelVersionDisplay.value, // 期望完成模型版本
    coordinationID: iterDeliverOrderForm.value.coordinationId, // 协调单号
    updateNotes: iterDeliverOrderForm.value.updateNotes, // 更新说明
    packageRequirement: iterDeliverOrderForm.value.packageRequirement, // 封装要求
    apiChanged: iterDeliverOrderForm.value.apiChanged, // 接口是否变化
    targetCustomer: iterDeliverOrderForm.value.targetCustomer, // 目标客户
    isCAEChecked: iterDeliverOrderForm.value.isCAEChecked, // 是否CAE检查
    hasSensitiveInfo: iterDeliverOrderForm.value.hasSensitiveInfo, // 是否包含敏感信息
    approverID: String(iterDeliverOrderForm.value.approverID), // 审批人ID
    startTime: new Date().toISOString().slice(0, 19).replace('T', ' '), // 开始时间(自动获取)
  })
  if (res?.data?.message) {
    ElMessage.success(res.data.message)
  }
  dialogIterDeliverVisible.value = false
  // 清空表单
  iterDeliverOrderForm.value = {
    modelId: '',
    modelVersionID: '',
    completeModelVersionFirst: '',
    completeModelVersionSecond: '',
    completeModelVersionNumber: '',
    coordinationId: '',
    updateNotes: '',
    packageRequirement: '',
    apiChanged: '',
    targetCustomer: '',
    isCAEChecked: '',
    hasSensitiveInfo: '',
    approverID: '',
  }
}

// -----------------功能开发工单-----------------
// 功能开发工单表单（上传后端）
const devOrderForm = ref({
  modelId: '', // 模型ID
  modelVersionID: '', // 模型版本ID
  completeModelVersionFirst: '', // 新增：第一位
  completeModelVersionSecond: '', // 新增：第二位
  completeModelVersionNumber: '', // 新增：完成模型版本号（只输入数字部分）
  featureDesc: '', // 功能描述
  approverID: '', // 审批人ID
})

// 功能开发工单完整版本号，直接取基准版本前三位
function getCompleteModelVersion(modelVersionID: string): string {
  const parts = modelVersionID.split('.')
  return parts.slice(0, 3).join('.')
}

// 提交功能开发工单的方法
async function submitDevOrder() {
  // 校验必填项
  if (
    !devOrderForm.value.modelId
    || !devOrderForm.value.modelVersionID
    || !devOrderForm.value.featureDesc
    || !devOrderForm.value.approverID
  ) {
    ElMessage.error('请完整填写所有必填项')
    return
  }

  // 提交到后端
  const res = await orderApi.submitDevOrder({
    orderID: '', // 新建时可为空或由后端生成
    status: '待审批', // 初始状态为待审批
    promoterID: String(currentUserId), // 当前用户ID
    modelID: devOrderForm.value.modelId, // 模型ID
    modelVersionID: devOrderForm.value.modelVersionID, // 模型版本ID
    matlab_version: getCompleteModelVersion(devOrderForm.value.modelVersionID), // 只取前三位
    featureDesc: devOrderForm.value.featureDesc, // 功能描述
    approverID: String(devOrderForm.value.approverID), // 审批人ID
    startTime: new Date().toISOString().slice(0, 19).replace('T', ' '), // 开始时间(自动获取)
  })

  // 显示后端返回的 message
  if (res?.data?.message) {
    ElMessage.success(res.data.message)
  }

  dialogDevVisible.value = false
  // 清空表单
  devOrderForm.value = {
    modelId: '',
    modelVersionID: '',
    completeModelVersionFirst: '',
    completeModelVersionSecond: '',
    completeModelVersionNumber: '',
    featureDesc: '',
    approverID: '',
  }
}

// -----------------其他工单-----------------
// 其他工单表单（上传后端）
const otherOrderForm = ref({
  modelId: '', // 模型ID
  modelVersionID: '', // 模型版本ID
  contentDesc: '', // 内容描述
  approverID: '', // 审批人ID
})

// 提交其他工单的方法
async function submitOtherOrder() {
  // 校验必填项
  if (
    !otherOrderForm.value.modelId
    || !otherOrderForm.value.modelVersionID
    || !otherOrderForm.value.contentDesc
    || !otherOrderForm.value.approverID
  ) {
    ElMessage.error('请完整填写所有必填项')
    return
  }
  // 提交到后端
  const res = await orderApi.submitOtherOrder({
    orderID: '', // 新建时可为空或由后端生成
    status: '待审批', // 初始状态为待审批
    promoterID: String(currentUserId), // 当前用户ID
    modelID: otherOrderForm.value.modelId, // 模型ID
    modelVersionID: otherOrderForm.value.modelVersionID, // 模型版本ID
    contentDesc: otherOrderForm.value.contentDesc, // 内容描述
    approverID: String(otherOrderForm.value.approverID), // 审批人ID
    startTime: new Date().toISOString().slice(0, 19).replace('T', ' '), // 开始时间(自动获取)
  })

  // 显示后端返回的 message
  if (res?.data?.message) {
    ElMessage.success(res.data.message)
  }

  dialogOtherVisible.value = false
  // 清空表单
  otherOrderForm.value = {
    modelId: '',
    modelVersionID: '',
    contentDesc: '',
    approverID: '',
  }
}

onMounted(() => {
  clientSuffixStore.fetchClientSuffixes() // 每次进入页面都请求后端获取客户后缀信息
  if (route.query.fromList === '1') {
    console.warn('从工单列表跳转到创建页面，复制工单类型:', route.query.copyType)
    // 判断是否为复制问题复现工单
    if (route.query.copyType === '问题复现') {
      dialogProblemVisible.value = true
      // 预填字段
      problemOrderForm.value.modelId = route.query.modelId as string || '' // 模型ATA章节号
      problemOrderForm.value.modelVersionID = route.query.modelVersionID as string || '' // 基准版本
      problemOrderForm.value.coordinationId = route.query.coordinationId as string || '' // 协调单号
      problemOrderForm.value.description = route.query.description as string || '' // 复现内容
    }
    else if (route.query.copyType === '版本迭代') {
      dialogIterVisible.value = true
      // 预填字段
      iterOrderForm.value.modelId = route.query.modelId as string || '' // 模型ATA章节号
      iterOrderForm.value.modelVersionID = route.query.modelVersionID as string || '' // 基准版本
      iterOrderForm.value.coordinationId = route.query.coordinationId as string || '' // 协调单号
      iterOrderForm.value.updateNotes = route.query.updateNotes as string || '' // 更新内容
      iterOrderForm.value.packageRequirement = route.query.packageRequirement as string || '' // 封装要求
      iterOrderForm.value.apiChanged = route.query.apiChanged as string || '' // 接口是否变化
    }
    else if (route.query.copyType === '交付发送') {
      dialogDeliverVisible.value = true
      // 预填字段
      deliverOrderForm.value.modelId = route.query.modelId as string || '' // 模型ATA章节号
      deliverOrderForm.value.modelVersionID = route.query.modelVersionID as string || '' // 基准版本
      deliverOrderForm.value.targetCustomer = route.query.targetCustomer as string || '' // 目标客户
      deliverOrderForm.value.isCAEChecked = route.query.isCAEChecked as string || '' // CAE认证
      deliverOrderForm.value.hasSensitiveInfo = route.query.hasSensitiveInfo as string || '' // 是否有敏感信息
    }
    else if (route.query.copyType === '版本迭代+交付发送') {
      dialogIterDeliverVisible.value = true
      // 预填字段
      iterDeliverOrderForm.value.modelId = route.query.modelId as string || '' // 模型ATA章节号
      iterDeliverOrderForm.value.modelVersionID = route.query.modelVersionID as string || '' // 基准版本
      iterDeliverOrderForm.value.coordinationId = route.query.coordinationId as string || '' // 协调单号
      iterDeliverOrderForm.value.updateNotes = route.query.updateNotes as string || '' // 更新内容
      iterDeliverOrderForm.value.packageRequirement = route.query.packageRequirement as string || '' // 封装要求
      iterDeliverOrderForm.value.apiChanged = route.query.apiChanged as string || '' // 接口是否变化
      iterDeliverOrderForm.value.targetCustomer = route.query.targetCustomer as string || '' // 目标客户
      iterDeliverOrderForm.value.isCAEChecked = route.query.isCAEChecked as string || '' // CAE认证
      iterDeliverOrderForm.value.hasSensitiveInfo = route.query.hasSensitiveInfo as string || '' // 是否有敏感信息
    }
    else if (route.query.copyType === '功能开发') {
      dialogDevVisible.value = true
      // 预填字段
      devOrderForm.value.modelId = route.query.modelId as string || '' // 模型ATA章节号
      devOrderForm.value.modelVersionID = route.query.modelVersionID as string || '' // 基准版本
      devOrderForm.value.featureDesc = route.query.featureDesc as string || '' // 功能描述
    }
    else if (route.query.copyType === '其他') {
      dialogOtherVisible.value = true
      // 预填字段
      otherOrderForm.value.modelId = route.query.modelId as string || '' // 模型ATA章节号
      otherOrderForm.value.modelVersionID = route.query.modelVersionID as string || '' // 基准版本
      otherOrderForm.value.contentDesc = route.query.contentDesc as string || '' // 内容描述
    }
  }
  else {
    console.warn('不是跳转')
  }
})

const router = useRouter()
function closeAllDialogs() {
  dialogProblemVisible.value = false
  dialogIterVisible.value = false
  dialogDeliverVisible.value = false
  dialogIterDeliverVisible.value = false
  dialogDevVisible.value = false
  dialogOtherVisible.value = false
  // 清除所有参数，只保留/order_create
  router.replace({ path: '/order_create' })
}

// ---------------下拉菜单部分------------------
const modelList = ref<string[]>(userStore.userModels) // 模型ID列表
const modelVersionList = ref<string[]>([]) // 模型版本ID列表
const customerList = ref<string[]>([]) // 客户列表

const approverList = ref<Array<{ id: string, name: string }>>([]) // 审批人列表

// 根据模型ID获取模型版本列表（支持传参，便于复用）
async function getModelVersionList(modelId?: string) {
  const id = modelId
  if (!id) {
    return
  }
  const res = await orderApi.fetchModelVersions(id)
  modelVersionList.value = res?.data?.list || []
}

// 获取审批人列表
async function fetchApproverList(modelId?: string) {
  if (!modelId) {
    ElMessage.warning('请先选择模型ID')
    return
  }

  try {
    const res = await orderApi.fetchApproverList(modelId)
    // 返回格式 [{ id: '工号', name: '姓名' }]
    approverList.value = res?.data?.list || []
  }
  catch (error) {
    console.error('获取审批人列表失败:', error)
    ElMessage.error('获取审批人列表失败')
    approverList.value = []
  }
}

// 获取客户列表
async function fetchCustomerList() {
  const res = await orderApi.fetchCustomerList?.()
  customerList.value = res?.data?.list || []
}
</script>

<template>
  <div>
    <!-- 顶部标题栏 -->
    <div class="mb-6 border border-gray-200 rounded-lg bg-white p-6 shadow-sm">
      <div class="flex items-center justify-between">
        <div class="flex items-center">
          <h1 class="text-2xl text-gray-800 font-bold">
            工单创建
          </h1>
        </div>
      </div>

      <!-- 文字提示行 -->
      <div class="mt-3 text-sm text-gray-600">
        本页面支持创建新的工单，填写相关信息后点击“提交”按钮即可完成工单创建。
      </div>
    </div>

    <!-- 工单区块 -->
    <div class="w-full flex flex-wrap gap-4 px-4">
      <!-- 问题复现工单区块 -->
      <FaPageMain class="m-0 max-w-[calc(50%-0.5rem)] min-w-[500px] flex-1" title-class="flex flex-wrap items-center justify-between gap-4">
        <template #title>
          <div class="flex items-center gap-2">
            <span class="text-xl text-black font-bold tracking-wide">问题复现工单</span>
          </div>
          <div class="ms-auto">
            <FaButton
              style=" color: #2563eb;background: #e0edff; border: none;"
              class="transition-colors hover:bg-blue-100 hover:text-blue-700"
              @click="dialogProblemVisible = true"
            >
              创建工单
            </FaButton>
          </div>
        </template>
        <div class="mb-6 flex items-start gap-3 border border-blue-100 rounded-lg bg-blue-50 p-4 shadow-sm">
          <FaIcon name="i-ep:info-filled" class="mt-1 text-2xl text-blue-400" />
          <div class="text-sm text-blue-700 leading-6">
            <div class="mb-1 font-semibold">
              工单填写说明：
            </div>
            <ol class="list-decimal pl-5 space-y-1">
              <li>模型（下拉选择）</li>
              <li>基准版本（下拉选择）</li>
              <li>对应协调单（单号）</li>
              <li>复现内容（文字描述）</li>
              <li>审批人（下拉选择）</li>
              <li>复现参考文件（上传附件）</li>
            </ol>
          </div>
        </div>
      </FaPageMain>

      <!-- 版本迭代工单区块 -->
      <FaPageMain class="m-0 max-w-[calc(50%-0.5rem)] min-w-[500px] flex-1" title-class="flex flex-wrap items-center justify-between gap-4">
        <template #title>
          <div class="flex items-center gap-2">
            <span class="text-xl text-black font-bold tracking-wide">版本迭代工单</span>
          </div>
          <div class="ms-auto">
            <FaButton
              style=" color: #2563eb;background: #e0edff; border: none;"
              class="transition-colors hover:bg-blue-100 hover:text-blue-700"
              @click="dialogIterVisible = true"
            >
              创建工单
            </FaButton>
          </div>
        </template>
        <div class="mb-6 flex items-start gap-3 border border-blue-100 rounded-lg bg-blue-50 p-4 shadow-sm">
          <FaIcon name="i-ep:info-filled" class="mt-1 text-2xl text-blue-400" />
          <div class="text-sm text-blue-700 leading-6">
            <div class="mb-1 font-semibold">
              工单填写说明：
            </div>
            <ol class="list-decimal pl-5 space-y-1">
              <li>模型（下拉选择）</li>
              <li>基准版本（下拉选择）</li>
              <li>对应协调单（单号）</li>
              <li>更新内容（文本）</li>
              <li>封装要求（文本）</li>
              <li>接口是否变化（是/否）</li>
              <li>审批人（下拉选择）</li>
            </ol>
          </div>
        </div>
      </FaPageMain>

      <!-- 交付发送类工单区块 -->
      <FaPageMain class="m-0 max-w-[calc(50%-0.5rem)] min-w-[500px] flex-1" title-class="flex flex-wrap items-center justify-between gap-4">
        <template #title>
          <div class="flex items-center gap-2">
            <span class="text-xl text-black font-bold tracking-wide">交付发送工单</span>
          </div>
          <div class="ms-auto">
            <FaButton
              style=" color: #2563eb;background: #e0edff; border: none;"
              class="transition-colors hover:bg-blue-100 hover:text-blue-700"
              @click="dialogDeliverVisible = true"
            >
              创建工单
            </FaButton>
          </div>
        </template>
        <div class="mb-6 flex items-start gap-3 border border-blue-100 rounded-lg bg-blue-50 p-4 shadow-sm">
          <FaIcon name="i-ep:info-filled" class="mt-1 text-2xl text-blue-400" />
          <div class="text-sm text-blue-700 leading-6">
            <div class="mb-1 font-semibold">
              工单填写说明：
            </div>
            <ol class="list-decimal pl-5 space-y-1">
              <li>模型（下拉选择）</li>
              <li>发送版本（下拉选择）</li>
              <li>目标客户（下拉选择）</li>
              <li>是否通过CAE-IPT平台验证（是/否）</li>
              <li>当前版本是否包含敏感信息（是/否）</li>
              <li>审批人（下拉选择）</li>
            </ol>
          </div>
        </div>
      </FaPageMain>

      <!-- 版本迭代+交付发送合并工单区块 -->
      <FaPageMain class="m-0 max-w-[calc(50%-0.5rem)] min-w-[500px] flex-1" title-class="flex flex-wrap items-center justify-between gap-4">
        <template #title>
          <div class="flex items-center gap-2">
            <span class="text-xl text-black font-bold tracking-wide">版本迭代+交付发送工单</span>
          </div>
          <div class="ms-auto">
            <FaButton
              style=" color: #2563eb;background: #e0edff; border: none;"
              class="transition-colors hover:bg-blue-100 hover:text-blue-700"
              @click="dialogIterDeliverVisible = true"
            >
              创建工单
            </FaButton>
          </div>
        </template>
        <div class="mb-6 flex items-start gap-3 border border-blue-100 rounded-lg bg-blue-50 p-4 shadow-sm">
          <FaIcon name="i-ep:info-filled" class="mt-1 text-2xl text-blue-400" />
          <div class="text-sm text-blue-700 leading-6">
            <div class="mb-1 font-semibold">
              工单填写说明：
            </div>
            <ol class="list-decimal pl-5 space-y-1">
              <li>模型（下拉选择）</li>
              <li>基准版本（下拉选择）</li>
              <li>对应协调单（单号）</li>
              <li>更新内容（文本）</li>
              <li>封装要求（文本）</li>
              <li>接口是否变化（是/否）</li>
              <li>目标客户（下拉选择）</li>
              <li>是否通过CAE-IPT平台验证（是/否）</li>
              <li>当前版本是否包含敏感信息（是/否）</li>
              <li>审批人（下拉选择）</li>
            </ol>
          </div>
        </div>
      </FaPageMain>

      <!-- 功能开发类工单区块 -->
      <FaPageMain class="m-0 max-w-[calc(50%-0.5rem)] min-w-[500px] flex-1" title-class="flex flex-wrap items-center justify-between gap-4">
        <template #title>
          <div class="flex items-center gap-2">
            <span class="text-xl text-black font-bold tracking-wide">功能开发工单</span>
          </div>
          <div class="ms-auto">
            <FaButton
              style=" color: #2563eb;background: #e0edff; border: none;"
              class="transition-colors hover:bg-blue-100 hover:text-blue-700"
              @click="dialogDevVisible = true"
            >
              创建工单
            </FaButton>
          </div>
        </template>
        <div class="mb-6 flex items-start gap-3 border border-blue-100 rounded-lg bg-blue-50 p-4 shadow-sm">
          <FaIcon name="i-ep:info-filled" class="mt-1 text-2xl text-blue-400" />
          <div class="text-sm text-blue-700 leading-6">
            <div class="mb-1 font-semibold">
              工单填写说明：
            </div>
            <ol class="list-decimal pl-5 space-y-1">
              <li>模型（下拉选择）</li>
              <li>基准版本（下拉选择）</li>
              <li>功能描述（文本）</li>
              <li>审批人（下拉选择）</li>
            </ol>
          </div>
        </div>
      </FaPageMain>

      <!-- 其他类工单区块 -->
      <FaPageMain class="m-0 max-w-[calc(50%-0.5rem)] min-w-[500px] flex-1" title-class="flex flex-wrap items-center justify-between gap-4">
        <template #title>
          <div class="flex items-center gap-2">
            <span class="text-xl text-black font-bold tracking-wide">其他类工单</span>
          </div>
          <div class="ms-auto">
            <FaButton
              style=" color: #2563eb;background: #e0edff; border: none;"
              class="transition-colors hover:bg-blue-100 hover:text-blue-700"
              @click="dialogOtherVisible = true"
            >
              创建工单
            </FaButton>
          </div>
        </template>
        <div class="mb-6 flex items-start gap-3 border border-blue-100 rounded-lg bg-blue-50 p-4 shadow-sm">
          <FaIcon name="i-ep:info-filled" class="mt-1 text-2xl text-blue-400" />
          <div class="text-sm text-blue-700 leading-6">
            <div class="mb-1 font-semibold">
              工单填写说明：
            </div>
            <ol class="list-decimal pl-5 space-y-1">
              <li>模型（下拉选择）</li>
              <li>基准版本（下拉选择）</li>
              <li>内容描述（文本）</li>
              <li>审批人（下拉选择）</li>
            </ol>
          </div>
        </div>
      </FaPageMain>

      <!-- 问题复现工单弹窗 -->
      <FaModal v-model="dialogProblemVisible" title="创建工单" width="500px" :close-on-click-modal="false" @close="closeAllDialogs">
        <el-form :model="problemOrderForm" label-width="120px">
          <!-- 模型ID下拉框 -->
          <el-form-item label="模型" required>
            <el-select
              v-model="problemOrderForm.modelId"
              placeholder="请选择模型"
              filterable
              clearable
              style="width: 100%;"
            >
              <el-option
                v-for="item in modelList"
                :key="item"
                :label="item"
                :value="item"
              />
            </el-select>
          </el-form-item>
          <!-- 基准版本下拉框 -->
          <el-form-item label="基准版本" required>
            <el-select
              v-model="problemOrderForm.modelVersionID"
              placeholder="请选择基准版本"
              filterable
              clearable
              style="width: 100%;"
              :disabled="!problemOrderForm.modelId"
              @visible-change="val => val && getModelVersionList(problemOrderForm.modelId)"
            >
              <el-option
                v-for="ver in modelVersionList"
                :key="ver"
                :label="ver"
                :value="ver"
              />
            </el-select>
          </el-form-item>
          <el-form-item label="协调单号">
            <el-input v-model="problemOrderForm.coordinationId" placeholder="请输入协调单号" />
          </el-form-item>
          <el-form-item label="复现内容" required>
            <el-input
              v-model="problemOrderForm.description"
              type="textarea"
              :rows="4"
              placeholder="请详细描述复现内容"
            />
          </el-form-item>
          <!-- 问题复现工单审批人ID -->
          <el-form-item label="审批人" required>
            <el-select
              v-model="problemOrderForm.approverID"
              placeholder="请选择审批人"
              filterable
              clearable
              style="width: 100%;"
              :disabled="!problemOrderForm.modelId"
              @visible-change="val => val && fetchApproverList(problemOrderForm.modelId)"
            >
              <el-option
                v-for="item in approverList"
                :key="item.id"
                :label="`${item.name} (${item.id})`"
                :value="item.id"
              />
            </el-select>
          </el-form-item>
          <el-form-item label="参考文件">
            <el-upload
              v-model:file-list="problemOrderForm.files"
              action="#"
              :auto-upload="false"
              :limit="1"
              list-type="text"
              :on-change="handleProblemFileChange"
            >
              <el-button type="primary">
                上传附件
              </el-button>
            </el-upload>
          </el-form-item>
        </el-form>
        <template #footer>
          <el-button @click="closeAllDialogs">
            取消
          </el-button>
          <el-button
            type="primary"
            :disabled="!problemOrderForm.modelId || !problemOrderForm.description || !problemOrderForm.modelVersionID || !problemOrderForm.approverID"
            @click="submitProblemOrder"
          >
            提交
          </el-button>
        </template>
      </FaModal>

      <!-- 版本迭代工单弹窗 -->
      <FaModal v-model="dialogIterVisible" title="创建版本迭代工单" width="500px" :close-on-click-overlay="false" @close="closeAllDialogs">
        <el-form :model="iterOrderForm" label-width="120px">
          <!-- 将模型ID输入框下拉菜单 -->
          <el-form-item label="模型" required>
            <el-select
              v-model="iterOrderForm.modelId"
              placeholder="请选择模型"
              filterable
              clearable
              style="width: 100%;"
            >
              <el-option
                v-for="item in modelList"
                :key="item"
                :label="item"
                :value="item"
              />
            </el-select>
          </el-form-item>
          <!-- 将基准版本输入框下拉菜单 -->
          <el-form-item label="基准版本" required>
            <el-select
              v-model="iterOrderForm.modelVersionID"
              placeholder="请选择基准版本"
              filterable
              clearable
              style="width: 100%;"
              :disabled="!iterOrderForm.modelId"
              @visible-change="val => val && getModelVersionList(iterOrderForm.modelId)"
            >
              <el-option
                v-for="ver in modelVersionList"
                :key="ver"
                :label="ver"
                :value="ver"
              />
            </el-select>
          </el-form-item>

          <!-- 完成模型版本输入（验证码样式） -->
          <el-form-item label="Matlab版本" required>
            <div class="version-input-container">
              <!-- 第一个数字（可编辑，默认基准版本第一位） -->
              <el-input
                v-model="iterOrderForm.completeModelVersionFirst"
                :placeholder="versionParts.first"
                maxlength="3"
                class="version-input"
                :disabled="!iterOrderForm.modelVersionID"
              />

              <!-- 第一个点 -->
              <div class="version-part static">
                .
              </div>

              <!-- 第二个数字（可编辑，默认基准版本第二位） -->
              <el-input
                v-model="iterOrderForm.completeModelVersionSecond"
                :placeholder="versionParts.second"
                maxlength="3"
                class="version-input"
                :disabled="!iterOrderForm.modelVersionID"
              />

              <!-- 第二个点 -->
              <div class="version-part static">
                .
              </div>

              <!-- 用户输入的数字（第三位） -->
              <el-input
                v-model="iterOrderForm.completeModelVersionNumber"
                :placeholder="versionNumberPlaceholder"
                maxlength="5"
                class="version-input"
                :disabled="!iterOrderForm.modelVersionID"
                @input="handleCompleteVersionInput"
              />
            </div>

            <!-- 完整版本预览 -->
            <div v-if="completeModelVersionDisplay" class="mt-3 text-sm text-gray-600">
              <span class="font-semibold">完整版本:</span>
              <span class="ml-2 rounded bg-blue-50 px-3 py-1 text-lg text-blue-700 font-bold font-mono">
                {{ completeModelVersionDisplay }}
              </span>
            </div>
          </el-form-item>

          <el-form-item label="协调单号">
            <el-input v-model="iterOrderForm.coordinationId" placeholder="请输入协调单号或NA" />
          </el-form-item>
          <el-form-item label="更新内容" required>
            <el-input
              v-model="iterOrderForm.updateNotes"
              type="textarea"
              :rows="3"
              placeholder="请输入更新内容"
            />
          </el-form-item>
          <el-form-item label="封装要求">
            <el-input
              v-model="iterOrderForm.packageRequirement"
              type="textarea"
              :rows="2"
              placeholder="请输入封装要求"
            />
          </el-form-item>
          <el-form-item required :label="`接口与${iterOrderForm.modelVersionID || '基准版本'}是否变化`" label-width="200px">
            <el-select v-model="iterOrderForm.apiChanged" placeholder="请选择">
              <el-option label="是" value="是" />
              <el-option label="否" value="否" />
            </el-select>
          </el-form-item>
          <!-- 版本迭代工单审批人ID -->
          <el-form-item label="审批人" required>
            <el-select
              v-model="iterOrderForm.approverID"
              placeholder="请选择审批人"
              filterable
              clearable
              style="width: 100%;"
              :disabled="!iterOrderForm.modelId"
              @visible-change="val => val && fetchApproverList(iterOrderForm.modelId)"
            >
              <el-option
                v-for="item in approverList"
                :key="item.id"
                :label="`${item.name} (${item.id})`"
                :value="item.id"
              />
            </el-select>
          </el-form-item>
        </el-form>
        <template #footer>
          <el-button @click="closeAllDialogs">
            取消
          </el-button>
          <el-button
            type="primary"
            :disabled="!iterOrderForm.modelId || !iterOrderForm.updateNotes || !iterOrderForm.apiChanged || !iterOrderForm.modelVersionID || !iterOrderForm.approverID"
            @click="submitIterOrder"
          >
            提交
          </el-button>
        </template>
      </FaModal>

      <!-- 交付发送工单弹窗 -->
      <FaModal v-model="dialogDeliverVisible" title="创建交付发送工单" width="500px" :close-on-click-overlay="false" @close="closeAllDialogs">
        <el-form :model="deliverOrderForm" label-width="140px">
          <el-form-item label="模型" required>
            <el-select
              v-model="deliverOrderForm.modelId"
              placeholder="请选择模型"
              filterable
              clearable
              style="width: 100%;"
            >
              <el-option
                v-for="item in modelList"
                :key="item"
                :label="item"
                :value="item"
              />
            </el-select>
          </el-form-item>
          <el-form-item label="发送版本" required>
            <el-select
              v-model="deliverOrderForm.modelVersionID"
              placeholder="请选择基准版本"
              filterable
              clearable
              style="width: 100%;"
              :disabled="!deliverOrderForm.modelId"
              @visible-change="val => val && getModelVersionList(deliverOrderForm.modelId)"
            >
              <el-option
                v-for="ver in modelVersionList"
                :key="ver"
                :label="ver"
                :value="ver"
              />
            </el-select>
          </el-form-item>
          <!-- 客户信息下拉菜单 -->
          <el-form-item label="目标客户" required>
            <el-select
              v-model="deliverOrderForm.targetCustomer"
              placeholder="请选择目标客户"
              filterable
              clearable
              style="width: 100%;"
              @visible-change="val => val && fetchCustomerList()"
            >
              <el-option
                v-for="item in customerList"
                :key="item"
                :label="item"
                :value="item"
              />
            </el-select>
            <!-- 显示选中客户的后缀信息 -->
            <div v-if="deliverOrderForm.targetCustomer && deliverCustomerSuffixes.length > 0" class="mt-2 text-sm text-gray-600">
              <span class="font-medium">该客户对应的后缀：</span>
              <span class="text-blue-600">{{ deliverCustomerSuffixes.join(', ') }}</span>
            </div>
            <div v-else-if="deliverOrderForm.targetCustomer && deliverCustomerSuffixes.length === 0" class="mt-2 text-sm text-orange-600">
              <span class="font-medium">注意：</span>该客户暂无对应的后缀信息
            </div>
          </el-form-item>
          <el-form-item label="CAE-IPT平台验证" required>
            <el-select v-model="deliverOrderForm.isCAEChecked" placeholder="请选择">
              <el-option label="是" value="是" />
              <el-option label="否" value="否" />
            </el-select>
          </el-form-item>
          <el-form-item label="是否包含敏感信息" required>
            <el-select v-model="deliverOrderForm.hasSensitiveInfo" placeholder="请选择">
              <el-option label="是" value="是" />
              <el-option label="否" value="否" />
            </el-select>
          </el-form-item>
          <!-- 交付发送工单审批人ID -->
          <el-form-item label="审批人" required>
            <el-select
              v-model="deliverOrderForm.approverID"
              placeholder="请选择审批人"
              filterable
              clearable
              style="width: 100%;"
              :disabled="!deliverOrderForm.modelId"
              @visible-change="val => val && fetchApproverList(deliverOrderForm.modelId)"
            >
              <el-option
                v-for="item in approverList"
                :key="item.id"
                :label="`${item.name} (${item.id})`"
                :value="item.id"
              />
            </el-select>
          </el-form-item>
        </el-form>
        <template #footer>
          <el-button @click="closeAllDialogs">
            取消
          </el-button>
          <el-button
            type="primary"
            :disabled="!deliverOrderForm.modelId || !deliverOrderForm.targetCustomer || !deliverOrderForm.isCAEChecked || !deliverOrderForm.modelVersionID || !deliverOrderForm.approverID"
            @click="handleDeliverOrderSubmit"
          >
            提交
          </el-button>
        </template>
      </FaModal>

      <!-- 版本迭代+交付发送工单弹窗 -->
      <FaModal v-model="dialogIterDeliverVisible" title="创建版本迭代+交付发送工单" width="500px" :close-on-click-modal="false" @close="closeAllDialogs">
        <el-form :model="iterDeliverOrderForm" label-width="140px">
          <el-form-item label="模型" required>
            <el-select
              v-model="iterDeliverOrderForm.modelId"
              placeholder="请选择模型"
              filterable
              clearable
              style="width: 100%;"
            >
              <el-option
                v-for="item in modelList"
                :key="item"
                :label="item"
                :value="item"
              />
            </el-select>
          </el-form-item>
          <el-form-item label="基准版本" required>
            <el-select
              v-model="iterDeliverOrderForm.modelVersionID"
              placeholder="请选择基准版本"
              filterable
              clearable
              style="width: 100%;"
              :disabled="!iterDeliverOrderForm.modelId"
              @visible-change="val => val && getModelVersionList(iterDeliverOrderForm.modelId)"
            >
              <el-option
                v-for="ver in modelVersionList"
                :key="ver"
                :label="ver"
                :value="ver"
              />
            </el-select>
          </el-form-item>

          <!-- 完成模型版本 - 验证码样式 -->
          <el-form-item label="Matlab版本" required>
            <div class="version-input-container">
              <!-- 第一个数字（可编辑，默认基准版本第一位） -->
              <el-input
                v-model="iterDeliverOrderForm.completeModelVersionFirst"
                :placeholder="iterDeliverVersionParts.first"
                maxlength="3"
                class="version-input"
                :disabled="!iterDeliverOrderForm.modelVersionID"
              />

              <!-- 第一个点 -->
              <div class="version-part static">
                .
              </div>

              <!-- 第二个数字（可编辑，默认基准版本第二位） -->
              <el-input
                v-model="iterDeliverOrderForm.completeModelVersionSecond"
                :placeholder="iterDeliverVersionParts.second"
                maxlength="3"
                class="version-input"
                :disabled="!iterDeliverOrderForm.modelVersionID"
              />

              <!-- 第二个点 -->
              <div class="version-part static">
                .
              </div>

              <!-- 用户输入的数字 -->
              <el-input
                v-model="iterDeliverOrderForm.completeModelVersionNumber"
                :placeholder="iterDeliverVersionNumberPlaceholder"
                maxlength="5"
                class="version-input"
                :disabled="!iterDeliverOrderForm.modelVersionID"
                @input="handleIterDeliverCompleteVersionInput"
              />
            </div>

            <!-- 完整版本预览 -->
            <div v-if="iterDeliverCompleteModelVersionDisplay" class="mt-3 text-sm text-gray-600">
              <span class="font-semibold">完整版本:</span>
              <span class="ml-2 rounded bg-blue-50 px-3 py-1 text-lg text-blue-700 font-bold font-mono">
                {{ iterDeliverCompleteModelVersionDisplay }}
              </span>
            </div>
          </el-form-item>

          <el-form-item label="协调单号">
            <el-input v-model="iterDeliverOrderForm.coordinationId" placeholder="请输入协调单号或NA" />
          </el-form-item>
          <el-form-item label="更新内容" required>
            <el-input
              v-model="iterDeliverOrderForm.updateNotes"
              type="textarea"
              :rows="3"
              placeholder="请输入更新内容"
            />
          </el-form-item>
          <el-form-item label="封装要求">
            <el-input
              v-model="iterDeliverOrderForm.packageRequirement"
              type="textarea"
              :rows="2"
              placeholder="请输入封装要求"
            />
          </el-form-item>
          <el-form-item required :label="`接口与${iterDeliverOrderForm.modelVersionID || '基准版本'}是否变化`" label-width="200px">
            <el-select v-model="iterDeliverOrderForm.apiChanged" placeholder="请选择">
              <el-option label="是" value="是" />
              <el-option label="否" value="否" />
            </el-select>
          </el-form-item>
          <!-- 客户信息下拉菜单 -->
          <el-form-item label="目标客户" required>
            <el-select
              v-model="iterDeliverOrderForm.targetCustomer"
              placeholder="请选择目标客户"
              filterable
              clearable
              style="width: 100%;"
              @visible-change="val => val && fetchCustomerList()"
            >
              <el-option
                v-for="item in customerList"
                :key="item"
                :label="item"
                :value="item"
              />
            </el-select>
            <!-- 显示选中客户的后缀信息 -->
            <div v-if="iterDeliverOrderForm.targetCustomer && iterDeliverCustomerSuffixes.length > 0" class="mt-2 text-sm text-gray-600">
              <span class="font-medium">该客户对应的后缀：</span>
              <span class="text-blue-600">{{ iterDeliverCustomerSuffixes.join(', ') }}</span>
            </div>
            <div v-else-if="iterDeliverOrderForm.targetCustomer && iterDeliverCustomerSuffixes.length === 0" class="mt-2 text-sm text-orange-600">
              <span class="font-medium">注意：</span>该客户暂无对应的后缀信息
            </div>
          </el-form-item>
          <el-form-item label="CAE-IPT平台验证" required>
            <el-select v-model="iterDeliverOrderForm.isCAEChecked" placeholder="请选择">
              <el-option label="是" value="是" />
              <el-option label="否" value="否" />
            </el-select>
          </el-form-item>
          <el-form-item label="是否包含敏感信息" required>
            <el-select v-model="iterDeliverOrderForm.hasSensitiveInfo" placeholder="请选择">
              <el-option label="是" value="true" />
              <el-option label="否" value="false" />
            </el-select>
          </el-form-item>
          <!-- 版本迭代+交付发送工单审批人ID -->
          <el-form-item label="审批人" required>
            <el-select
              v-model="iterDeliverOrderForm.approverID"
              placeholder="请选择审批人"
              filterable
              clearable
              style="width: 100%;"
              :disabled="!iterDeliverOrderForm.modelId"
              @visible-change="val => val && fetchApproverList(iterDeliverOrderForm.modelId)"
            >
              <el-option
                v-for="item in approverList"
                :key="item.id"
                :label="`${item.name} (${item.id})`"
                :value="item.id"
              />
            </el-select>
          </el-form-item>
        </el-form>
        <template #footer>
          <el-button @click="closeAllDialogs">
            取消
          </el-button>
          <el-button
            type="primary"
            :disabled="!iterDeliverOrderForm.modelId
              || !iterDeliverOrderForm.modelVersionID
              || !iterDeliverOrderForm.completeModelVersionNumber
              || !iterDeliverOrderForm.updateNotes
              || !iterDeliverOrderForm.apiChanged
              || !iterDeliverOrderForm.targetCustomer
              || !iterDeliverOrderForm.isCAEChecked
              || !iterDeliverOrderForm.approverID"
            @click="submitIterDeliverOrder"
          >
            提交
          </el-button>
        </template>
      </FaModal>

      <!-- 功能开发工单弹窗 -->
      <FaModal v-model="dialogDevVisible" title="创建功能开发工单" width="500px" :close-on-click-modal="false" @close="closeAllDialogs">
        <el-form :model="devOrderForm" label-width="120px">
          <el-form-item label="模型" required>
            <el-select
              v-model="devOrderForm.modelId"
              placeholder="请选择模型"
              filterable
              clearable
              style="width: 100%;"
            >
              <el-option
                v-for="item in modelList"
                :key="item"
                :label="item"
                :value="item"
              />
            </el-select>
          </el-form-item>
          <el-form-item label="基准版本" required>
            <el-select
              v-model="devOrderForm.modelVersionID"
              placeholder="请选择基准版本"
              filterable
              clearable
              style="width: 100%;"
              :disabled="!devOrderForm.modelId"
              @visible-change="val => val && getModelVersionList(devOrderForm.modelId)"
            >
              <el-option
                v-for="ver in modelVersionList"
                :key="ver"
                :label="ver"
                :value="ver"
              />
            </el-select>
          </el-form-item>

          <el-form-item label="功能描述" required>
            <el-input
              v-model="devOrderForm.featureDesc"
              type="textarea"
              :rows="3"
              placeholder="请输入功能描述"
            />
          </el-form-item>
          <!-- 功能开发工单审批人ID -->
          <el-form-item label="审批人" required>
            <el-select
              v-model="devOrderForm.approverID"
              placeholder="请选择审批人"
              filterable
              clearable
              style="width: 100%;"
              :disabled="!devOrderForm.modelId"
              @visible-change="val => val && fetchApproverList(devOrderForm.modelId)"
            >
              <el-option
                v-for="item in approverList"
                :key="item.id"
                :label="`${item.name} (${item.id})`"
                :value="item.id"
              />
            </el-select>
          </el-form-item>
        </el-form>
        <template #footer>
          <el-button @click="closeAllDialogs">
            取消
          </el-button>
          <el-button
            type="primary"
            :disabled="!devOrderForm.modelId || !devOrderForm.modelVersionID || !devOrderForm.featureDesc || !devOrderForm.approverID"
            @click="submitDevOrder"
          >
            提交
          </el-button>
        </template>
      </FaModal>

      <!-- 其他类工单弹窗 -->
      <FaModal v-model="dialogOtherVisible" title="创建其他类工单" width="500px" :close-on-click-modal="false" @close="closeAllDialogs">
        <el-form :model="otherOrderForm" label-width="120px">
          <el-form-item label="模型" required>
            <el-select
              v-model="otherOrderForm.modelId"
              placeholder="请选择模型"
              filterable
              clearable
              style="width: 100%;"
            >
              <el-option
                v-for="item in modelList"
                :key="item"
                :label="item"
                :value="item"
              />
            </el-select>
          </el-form-item>
          <el-form-item label="基准版本" required>
            <el-select
              v-model="otherOrderForm.modelVersionID"
              placeholder="请选择基准版本"
              filterable
              clearable
              style="width: 100%;"
              :disabled="!otherOrderForm.modelId"
              @visible-change="val => val && getModelVersionList(otherOrderForm.modelId)"
            >
              <el-option
                v-for="ver in modelVersionList"
                :key="ver"
                :label="ver"
                :value="ver"
              />
            </el-select>
          </el-form-item>
          <el-form-item label="内容描述" required>
            <el-input
              v-model="otherOrderForm.contentDesc"
              type="textarea"
              :rows="3"
              placeholder="请输入内容描述"
            />
          </el-form-item>
          <!-- 其他工单审批人ID -->
          <el-form-item label="审批人" required>
            <el-select
              v-model="otherOrderForm.approverID"
              placeholder="请选择审批人"
              filterable
              clearable
              style="width: 100%;"
              :disabled="!otherOrderForm.modelId"
              @visible-change="val => val && fetchApproverList(otherOrderForm.modelId)"
            >
              <el-option
                v-for="item in approverList"
                :key="item.id"
                :label="`${item.name} (${item.id})`"
                :value="item.id"
              />
            </el-select>
          </el-form-item>
        </el-form>
        <template #footer>
          <el-button @click="closeAllDialogs">
            取消
          </el-button>
          <el-button
            type="primary"
            :disabled="!otherOrderForm.modelId || !otherOrderForm.modelVersionID || !otherOrderForm.contentDesc || !otherOrderForm.approverID"
            @click="submitOtherOrder"
          >
            提交
          </el-button>
        </template>
      </FaModal>

      <!-- 交付发送类工单客户名称与版本后缀是否匹配二次确认弹窗 -->
      <el-dialog
        v-model="deliverConfirmDialogVisible"
        title="提交确认"
        width="400px"
        :close-on-click-modal="false"
      >
        <div class="mb-4 text-base text-gray-700">
          <p>
            <strong>目标客户：</strong> {{ deliverConfirmInfo.customer }}
          </p>
          <p>
            <strong>模型版本后缀：</strong> {{ deliverConfirmInfo.suffix }}
          </p>
          <p v-if="!deliverConfirmInfo.isMatch" class="mt-2 text-red-600">
            警告：目标客户与模型版本后缀不匹配，是否继续提交？
          </p>
          <p v-else class="mt-2 text-green-600">
            客户与模型版本后缀匹配，是否确认提交？
          </p>
        </div>
        <template #footer>
          <el-button @click="deliverConfirmDialogVisible = false">
            取消
          </el-button>
          <el-button type="primary" @click="confirmDeliverOrderSubmit">
            确认提交
          </el-button>
        </template>
      </el-dialog>

      <!-- 版本迭代+交付发送工单二次确认弹窗 -->
      <el-dialog
        v-model="iterDeliverConfirmDialogVisible"
        title="提交确认"
        width="400px"
        :close-on-click-modal="false"
      >
        <div class="mb-4 text-base text-gray-700">
          <p>
            <strong>目标客户：</strong> {{ iterDeliverConfirmInfo.customer }}
          </p>
          <p>
            <strong>模型版本后缀：</strong> {{ iterDeliverConfirmInfo.suffix }}
          </p>
          <p v-if="!iterDeliverConfirmInfo.isMatch" class="mt-2 text-red-600">
            警告：目标客户与模型版本后缀不匹配，是否继续提交？
          </p>
          <p v-else class="mt-2 text-green-600">
            客户与模型版本后缀匹配，是否确认提交？
          </p>
        </div>
        <template #footer>
          <el-button @click="iterDeliverConfirmDialogVisible = false">
            取消
          </el-button>
          <el-button type="primary" @click="confirmIterDeliverOrderSubmit">
            确认提交
          </el-button>
        </template>
      </el-dialog>
    </div>
  </div>
</template>

<style scoped>
/* 美化工单区域 */
.bg-blue-50 {
  background-color: #f0f6ff;
}

.border-blue-100 {
  border-color: #cfe2ff;
}

.text-blue-700 {
  color: #1d4ed8;
}

.text-blue-400 {
  color: #60a5fa;
}

.rounded-lg {
  border-radius: 0.75rem;
}

.shadow-sm {
  box-shadow: 0 1px 2px 0 rgb(0 0 0 / 5%);
}

.text-emphasis {
  text-emphasis-style: "❤";
}

.title-info {
  --uno: flex items-center gap-4;

  img {
    --uno: block w-12 h-12;
  }

  h1 {
    --uno: m-0 text-2xl;
  }

  h2 {
    --uno: m-0 text-base text-secondary-foreground/50 font-normal;
  }
}

.custom-progress {
  min-width: 350px;
}

.custom-progress .bg-gradient-to-br {
  background: linear-gradient(135deg, #2563eb 60%, #60a5fa 100%) !important;
}

.fixed-select-dropdown {
  max-height: 320px !important; /* 约10条，每条32px */
  overflow-y: auto !important;
}

/* 版本输入容器 - 验证码样式 */
.version-input-container {
  display: flex;
  gap: 8px;
  align-items: center;
  justify-content: flex-start;
}

/* 版本部分样式 */
.version-part {
  display: flex;
  align-items: center;
  justify-content: center;
  width: 40px;
  height: 40px;
  font-family: "Courier New", monospace;
  font-size: 18px;
  font-weight: bold;
  background-color: #f9fafb;
  border: 2px solid #e5e7eb;
  border-radius: 8px;
}

/* 静态部分（V和点号） */
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
  width: 50px;
}

.version-input :deep(.el-input__wrapper) {
  height: 40px;
  background-color: #fff;
  border: 2px solid #3b82f6;
  border-radius: 8px;
  box-shadow: 0 0 0 3px rgb(59 130 246 / 10%);
  transition: all 0.2s ease;
}

.version-input :deep(.el-input__wrapper:hover) {
  border-color: #2563eb;
  box-shadow: 0 0 0 3px rgb(37 99 235 / 15%);
}

.version-input :deep(.el-input__wrapper.is-focus) {
  border-color: #1d4ed8;
  box-shadow: 0 0 0 3px rgb(29 78 216 / 20%);
}

.version-input :deep(.el-input__inner) {
  padding: 0;
  font-family: "Courier New", monospace;
  font-size: 18px;
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
    width: 32px;
    height: 32px;
    font-size: 14px;
  }

  .version-input {
    width: 40px;
  }

  .version-input :deep(.el-input__wrapper) {
    height: 32px;
  }

  .version-input :deep(.el-input__inner) {
    font-size: 14px;
  }
}
</style>
