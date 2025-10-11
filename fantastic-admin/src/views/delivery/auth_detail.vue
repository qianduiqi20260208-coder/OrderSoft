<route lang="yaml">
meta:
  title: 授权详情
</route>

<script setup lang="ts">
import { ElMessage, ElMessageBox } from 'element-plus'
import { onMounted, ref } from 'vue'
import { useRoute, useRouter } from 'vue-router'
import deliveryApi from '@/api/modules/delivery'

const route = useRoute() // 获取当前路由信息
const router = useRouter() // 获取路由实例
const clientName_ = ref('') // 客户名称
const dongleCount_ = ref('') // 加密狗数量
const validCount_ = ref('') // 有效授权数量
const expiringCount_ = ref('') // 临期授权数量（5天内过期）
const expiredCount_ = ref('') // 过期授权数量
const totalAuthCount_ = ref('') // 总授权ID数量
const authDetail = ref<AuthDetail | null>(null) // 授权详情数据
const loading = ref(false) // 加载状态

const expandedMap = ref<Record<string, boolean>>({}) // 展开状态管理

// 授权详情数据结构
interface AuthDetail {
  clientName: string // 客户名称
  dongleCount: number // 加密狗数量
  shellNumbers: ShellInfo[] // 外壳号信息列表
}

// 外壳号信息
interface ShellInfo {
  shellNumber: string // 外壳号
  deviceType: string // 客户设备类型 lab/IPT/FTD/FFS
  deviceNote: string // 客户设备备注
  contractName: string // 合同名称
  contractNumber: string // 合同编号
  authorizationList: AuthorizationInfo[] // 该外壳号下的授权信息列表
  authCount: number // 该外壳号下的总授权数量
  inTime: string // 入库时间
  outTime: string // 出库时间
  isFullyLoaded: boolean // 标识是否已加载完整数据
}

// 授权信息
interface AuthorizationInfo {
  authId: string // 授权ID
  startDate: string // 授权开始日期 (YYYY-MM-DD)
  endDate: string // 授权结束日期 (YYYY-MM-DD)
  authType: string // 授权类型 本地锁/网络锁/软锁授权
  authNote: string // 授权备注
  status: string // 授权状态（有效、临期、过期）
}

// 计算剩余天数
function calculateRemainingDays(endDate: string): string {
  const today = new Date()
  const end = new Date(endDate)

  // 将时间重置为当天0点，避免时间差的影响
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

  // 将时间重置为当天0点，避免时间差的影响
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

// // 获取状态圆点颜色
// function getStatusDotColor(status: string): string {
//   switch (status) {
//     case '有效': return 'bg-green-500'
//     case '临期': return 'bg-orange-500'
//     case '过期': return 'bg-gray-400'
//     default: return 'bg-gray-400'
//   }
// }

// 获取外壳号状态圆点颜色（考虑懒加载情况）
function getShellStatusDotColor(shell: ShellInfo): string {
  // 如果未完全加载，只基于第一条授权记录判断
  if (!shell.isFullyLoaded && shell.authorizationList.length === 1) {
    const auth = shell.authorizationList[0]
    const today = new Date()
    const end = new Date(auth.endDate)

    today.setHours(0, 0, 0, 0)
    end.setHours(0, 0, 0, 0)

    const diffTime = end.getTime() - today.getTime()
    const diffDays = Math.ceil(diffTime / (1000 * 60 * 60 * 24))

    if (diffDays > 5) {
      return 'bg-green-500' // 有效
    }
    else if (diffDays >= 0 && diffDays <= 5) {
      return 'bg-orange-500' // 临期
    }
    else {
      return 'bg-gray-400' // 过期
    }
  }

  // 如果已完全加载，使用原有逻辑
  let hasValid = false
  let hasExpiring = false

  for (const auth of shell.authorizationList) {
    const today = new Date()
    const end = new Date(auth.endDate)

    today.setHours(0, 0, 0, 0)
    end.setHours(0, 0, 0, 0)

    const diffTime = end.getTime() - today.getTime()
    const diffDays = Math.ceil(diffTime / (1000 * 60 * 60 * 24))

    if (diffDays > 5) {
      hasValid = true
    }
    else if (diffDays >= 0 && diffDays <= 5) {
      hasExpiring = true
    }
  }

  if (hasValid) {
    return 'bg-green-500'
  }
  else if (hasExpiring) {
    return 'bg-orange-500'
  }
  else {
    return 'bg-gray-400'
  }
}

// 根据结束日期计算授权状态
function calculateAuthStatus(endDate: string): '有效' | '临期' | '过期' {
  const today = new Date()
  const end = new Date(endDate)

  // 将时间重置为当天0点，避免时间差的影响
  today.setHours(0, 0, 0, 0)
  end.setHours(0, 0, 0, 0)

  const diffTime = end.getTime() - today.getTime()
  const diffDays = Math.ceil(diffTime / (1000 * 60 * 60 * 24))

  if (diffDays > 5) {
    return '有效'
  }
  else if (diffDays >= 0 && diffDays <= 5) {
    return '临期'
  }
  else {
    return '过期'
  }
}

// ------------交付外壳弹窗---------------
// 交付外壳弹窗相关状态
const deliverShellDialogVisible = ref(false)
const deliverShellForm = ref({
  shellNumber: '', // 外壳号
  deviceType: '', // 设备类型
  deviceNote: '', // 设备备注
  contractName: '', // 合同名称
  contractNumber: '', // 合同编号
})

// 可交付外壳号列表相关状态
const availableShells = ref<string[]>([])
const shellsLoading = ref(false)

// 设备类型选项
const deviceTypeOptions = [
  { label: 'lab', value: 'lab' },
  { label: 'IPT', value: 'IPT' },
  { label: 'FTD', value: 'FTD' },
  { label: 'FFS', value: 'FFS' },
]

// 获取可交付外壳号列表
async function fetchAvailableShells() {
  shellsLoading.value = true
  try {
    const res = await deliveryApi.getAvailableShells()

    // 根据您的后端数据格式修改解析逻辑
    if (res?.data?.shells && Array.isArray(res.data.shells)) {
      // 提取所有的 shellNumber
      availableShells.value = res.data.shells.map((shell: any) => shell.shellNumber)
    }
    else {
      availableShells.value = []
      ElMessage.warning('暂无可交付的外壳号')
    }
  }
  catch (error) {
    console.error('获取可交付外壳号失败:', error)
    ElMessage.error('获取可交付外壳号失败')
    availableShells.value = []
  }
  finally {
    shellsLoading.value = false
  }
}
// 交付外壳处理函数
async function handleDeliverShell() {
  // 重置表单
  deliverShellForm.value = {
    shellNumber: '',
    deviceType: '',
    deviceNote: '',
    contractName: '',
    contractNumber: '',
  }

  // 获取可交付外壳号列表
  await fetchAvailableShells()

  deliverShellDialogVisible.value = true
}

// 确认交付外壳
async function handleConfirmDeliverShell() {
  try {
    // 表单验证
    if (!deliverShellForm.value.shellNumber.trim()) {
      ElMessage.warning('请选择外壳号')
      return
    }
    if (!deliverShellForm.value.deviceType) {
      ElMessage.warning('请选择设备类型')
      return
    }
    if (!deliverShellForm.value.deviceNote.trim()) {
      ElMessage.warning('请填写设备备注')
      return
    }

    // 调用交付外壳API - 简化数据结构
    const res = await deliveryApi.deliverShell({
      clientName: clientName_.value, // 客户名称
      shellNumber: deliverShellForm.value.shellNumber, // 选中的外壳号
      deviceType: deliverShellForm.value.deviceType, // 设备类型
      deviceNote: deliverShellForm.value.deviceNote, // 设备备注
      contractName: deliverShellForm.value.contractName, // 合同名称
      contractNumber: deliverShellForm.value.contractNumber, // 合同编号
    })

    if (res?.data?.success) {
      // 关闭弹窗
      deliverShellDialogVisible.value = false

      // 显示成功提示
      ElMessage.success(res.data.message || `外壳号 ${deliverShellForm.value.shellNumber} 交付成功`)

      // 刷新页面数据
      if (clientName_.value) {
        await fetchAuthDetail(clientName_.value)
      }
    }
    else {
      ElMessage.error(res?.data?.message || '交付外壳失败')
    }
  }
  catch (error: any) {
    console.error('交付外壳失败:', error)

    // 处理不同类型的错误
    if (error?.response?.data?.message) {
      ElMessage.error(error.response.data.message)
    }
    else if (error?.message) {
      ElMessage.error(error.message)
    }
    else {
      ElMessage.error('交付外壳失败，请稍后重试')
    }
  }
}

// 取消交付外壳
function handleCancelDeliverShell() {
  deliverShellDialogVisible.value = false
  deliverShellForm.value = {
    shellNumber: '',
    deviceType: '',
    deviceNote: '',
    contractName: '',
    contractNumber: '',
  }
}

// ------------归还外壳弹窗---------------
// 归还外壳弹窗相关状态
const returnShellDialogVisible = ref(false)
const returnShellForm = ref({
  operationType: '', // 新增：操作类型
  shellNumber: '', // 选择的外壳号
  returnDate: '', // 入库时间
  remark: '', // 新增：外壳号备注（仅在丢失和损坏时显示）
})

// 新增：操作类型选项
const operationTypeOptions = [
  { label: '归还外壳', value: 'return', description: '正常归还外壳到库存' },
  { label: '外壳丢失', value: 'lost', description: '外壳丢失，需要填写备注说明' },
  { label: '外壳损坏', value: 'damaged', description: '外壳损坏，需要填写备注说明' },
]

// 新增：获取当前日期时间
function getCurrentDateTime() {
  const now = new Date()
  const year = now.getFullYear()
  const month = String(now.getMonth() + 1).padStart(2, '0')
  const day = String(now.getDate()).padStart(2, '0')
  const hours = String(now.getHours()).padStart(2, '0')
  const minutes = String(now.getMinutes()).padStart(2, '0')
  const seconds = String(now.getSeconds()).padStart(2, '0')
  return `${year}-${month}-${day} ${hours}:${minutes}:${seconds}`
}

// 判断是否需要显示备注字段
const shouldShowRemark = computed(() => {
  return returnShellForm.value.operationType === 'lost' || returnShellForm.value.operationType === 'damaged'
})

// 新增：获取操作类型的标题文本
const getOperationTitle = computed(() => {
  switch (returnShellForm.value.operationType) {
    case 'return': return '归还外壳'
    case 'lost': return '外壳丢失'
    case 'damaged': return '外壳损坏'
    default: return '外壳操作'
  }
})

// 新增：获取操作类型的说明文本
const getOperationDescription = computed(() => {
  switch (returnShellForm.value.operationType) {
    case 'return': return '• 选择要归还的外壳号，归还后将回收到库存中<br>• 归还时如有有效授权，系统会提示确认<br>• 请准确选择入库时间'
    case 'lost': return '• 选择丢失的外壳号，标记为丢失状态<br>• 需要填写丢失原因和相关备注<br>• 请准确选择发现丢失的时间'
    case 'damaged': return '• 选择损坏的外壳号，标记为损坏状态<br>• 需要填写损坏原因和处理建议<br>• 请准确选择发现损坏的时间'
    default: return '请先选择操作类型'
  }
})

// 新增：获取时间字段的标签文本
const getDateLabel = computed(() => {
  switch (returnShellForm.value.operationType) {
    case 'return': return '入库时间'
    case 'lost': return '丢失时间'
    case 'damaged': return '损坏时间'
    default: return '时间'
  }
})

// 修改：操作类型变化处理
function handleOperationTypeChange(operationType: string) {
  // 重置表单其他字段
  returnShellForm.value.shellNumber = ''
  returnShellForm.value.remark = ''

  // 根据操作类型设置时间
  if (operationType === 'return') {
    // 归还外壳：固定为当前时间
    returnShellForm.value.returnDate = getCurrentDateTime()
  }
  else {
    // 丢失/损坏：清空时间，让用户选择
    returnShellForm.value.returnDate = ''
  }

  console.warn('选择的操作类型:', operationType)
}

// 获取可归还的外壳号列表（当前客户的外壳号）
const getReturnableShells = computed(() => {
  if (!authDetail.value) {
    return []
  }
  return authDetail.value.shellNumbers.map(shell => ({
    label: `${shell.shellNumber} (${shell.deviceType})`,
    value: shell.shellNumber,
    deviceType: shell.deviceType,
    deviceNote: shell.deviceNote,
    contractName: shell.contractName,
    contractNumber: shell.contractNumber,
    authCount: shell.authCount,
  }))
})

// 归还外壳处理函数
function handleReturnShell() {
  // 重置表单
  returnShellForm.value = {
    operationType: '',
    shellNumber: '',
    returnDate: '', // 初始为空，根据操作类型设置
    remark: '',
  }
  returnShellDialogVisible.value = true
}

// 确认归还外壳
async function handleConfirmReturnShell() {
  try {
    // 表单验证
    if (!returnShellForm.value.operationType) {
      ElMessage.warning('请选择操作类型')
      return
    }
    if (!returnShellForm.value.shellNumber) {
      ElMessage.warning('请选择要操作的外壳号')
      return
    }
    if (!returnShellForm.value.returnDate) {
      ElMessage.warning(`请选择${getDateLabel.value}`)
      return
    }
    if (shouldShowRemark.value && !returnShellForm.value.remark.trim()) {
      ElMessage.warning('请填写外壳号备注')
      return
    }

    // 日期验证（仅对丢失和损坏操作验证不能选择未来日期）
    if (returnShellForm.value.operationType !== 'return') {
      const today = new Date().toISOString().split('T')[0]
      const selectedDate = returnShellForm.value.returnDate.split(' ')[0]
      if (selectedDate > today) {
        ElMessage.warning(`${getDateLabel.value}不能选择未来日期`)
        return
      }
    }

    // 获取外壳号详细信息
    const selectedShell = authDetail.value?.shellNumbers.find(
      shell => shell.shellNumber === returnShellForm.value.shellNumber,
    )

    if (!selectedShell) {
      ElMessage.error('未找到选择的外壳号信息')
      return
    }

    // 对于归还操作，检查是否有有效授权
    if (returnShellForm.value.operationType === 'return') {
      const hasValidAuth = selectedShell.authorizationList.some((auth) => {
        const endDate = new Date(auth.endDate)
        const today = new Date()
        return endDate >= today
      })

      if (hasValidAuth) {
        const confirmed = await ElMessageBox.confirm(
          '该外壳号下还有有效的授权信息，确定要归还吗？归还后相关授权将失效。',
          '确认归还',
          {
            confirmButtonText: '确定归还',
            cancelButtonText: '取消',
            type: 'warning',
          },
        ).catch(() => false)

        if (!confirmed) {
          return
        }
      }
    }

    // 对于丢失和损坏操作，需要额外确认
    if (returnShellForm.value.operationType === 'lost' || returnShellForm.value.operationType === 'damaged') {
      const operationText = returnShellForm.value.operationType === 'lost' ? '丢失' : '损坏'
      const confirmed = await ElMessageBox.confirm(
        `确定要将外壳号 ${returnShellForm.value.shellNumber} 标记为${operationText}状态吗？此操作不可逆。`,
        `确认${operationText}`,
        {
          confirmButtonText: `确定${operationText}`,
          cancelButtonText: '取消',
          type: 'warning',
        },
      ).catch(() => false)

      if (!confirmed) {
        return
      }
    }

    // 新增：操作类型转换函数
    function convertOperationType(type: string): string {
      switch (type) {
        case 'return': return '入库'
        case 'lost': return '丢失'
        case 'damaged': return '损坏'
        default: return type
      }
    }

    // 调用操作API
    const res = await deliveryApi.returnShell({
      clientName: clientName_.value, // 客户名称
      operationType: convertOperationType(returnShellForm.value.operationType), // 转换为中文操作类型
      shellNumber: returnShellForm.value.shellNumber,
      inTime: returnShellForm.value.returnDate,
      outTime: selectedShell.outTime,
      remark: returnShellForm.value.remark,
    })

    if (res?.data?.success) {
      // 关闭弹窗
      returnShellDialogVisible.value = false

      // 显示成功提示
      const operationText = getOperationTitle.value
      ElMessage.success(res.data.message || `外壳号 ${returnShellForm.value.shellNumber} ${operationText}操作成功`)

      // 刷新页面数据
      if (clientName_.value) {
        await fetchAuthDetail(clientName_.value)
      }
    }
    else {
      ElMessage.error(res?.data?.message || `${getOperationTitle.value}操作失败`)
    }
  }
  catch (error: any) {
    console.error(`${getOperationTitle.value}操作失败:`, error)

    // 处理不同类型的错误
    if (error?.response?.data?.message) {
      ElMessage.error(error.response.data.message)
    }
    else if (error?.message) {
      ElMessage.error(error.message)
    }
    else {
      ElMessage.error(`${getOperationTitle.value}操作失败，请稍后重试`)
    }
  }
}

// 取消归还外壳
function handleCancelReturnShell() {
  returnShellDialogVisible.value = false
  returnShellForm.value = {
    operationType: '',
    shellNumber: '',
    returnDate: '',
    remark: '',
  }
}

// 外壳号选择变化时的处理 - 简化或移除
function handleShellNumberChange(shellNumber: string) {
  // 由于不再需要自动填充归还备注，这个函数可以简化或移除
  console.warn('选择的外壳号:', shellNumber)
}

// ---------编辑外壳号相关信息----------------
// 编辑弹窗相关状态
const editDialogVisible = ref(false)
const editingShell = ref<ShellInfo | null>(null)
const editForm = ref({
  deviceType: '',
  deviceNote: '',
  contractName: '',
  contractNumber: '',
})

// 编辑外壳号信息处理函数
function handleEditShell(shellNumber: string) {
  const shell = authDetail.value?.shellNumbers.find(s => s.shellNumber === shellNumber)
  if (shell) {
    editingShell.value = shell
    editForm.value.deviceType = shell.deviceType
    editForm.value.deviceNote = shell.deviceNote
    editForm.value.contractName = shell.contractName
    editForm.value.contractNumber = shell.contractNumber
    editDialogVisible.value = true
  }
}

// 编辑外壳号信息提交
async function handleConfirmEdit() {
  if (!editingShell.value) {
    return
  }

  try {
    // 调用更新API
    const res = await deliveryApi.updateShellInfo({
      clientName: clientName_.value, // 客户名称
      shellNumber: editingShell.value.shellNumber, // 外壳号
      deviceType: editForm.value.deviceType, // 设备类型
      deviceNote: editForm.value.deviceNote, // 设备备注
      contractName: editForm.value.contractName, // 合同名称
      contractNumber: editForm.value.contractNumber, // 合同编号
    })

    // 更新本地数据
    if (authDetail.value) {
      const shell = authDetail.value.shellNumbers.find(s => s.shellNumber === editingShell.value!.shellNumber)
      if (shell) {
        shell.deviceType = editForm.value.deviceType
        shell.deviceNote = editForm.value.deviceNote
        shell.contractName = editForm.value.contractName
        shell.contractNumber = editForm.value.contractNumber
      }
    }

    // 关闭弹窗
    editDialogVisible.value = false

    // 显示成功提示
    ElMessage.success(res.data.message || '外壳号信息更新成功')
  }
  catch (error) {
    console.error('更新外壳号信息失败:', error)
  }
}

// 取消编辑
function handleCancelEdit() {
  editDialogVisible.value = false
  editingShell.value = null
  editForm.value = {
    deviceType: '',
    deviceNote: '',
    contractName: '',
    contractNumber: '',
  }
}

// ------------新建授权弹窗---------------
// 新建授权弹窗相关状态
const createAuthDialogVisible = ref(false)
const createAuthShellNumber = ref('')
const createAuthForm = ref({
  authId: '',
  authType: '',
  startDate: '',
  endDate: '',
  authNote: '',
})

// 授权类型选项（根据数据库设计）
const authTypeOptions = [
  { label: '本地锁', value: '本地锁' },
  { label: '网络锁', value: '网络锁' },
  { label: '软锁授权', value: '软锁授权' },
]

// 为特定外壳号新建授权处理函数
function handleCreateAuthForShell(shellNumber: string) {
  createAuthShellNumber.value = shellNumber
  createAuthForm.value = {
    authId: '',
    authType: '',
    startDate: '',
    endDate: '',
    authNote: '',
  }
  createAuthDialogVisible.value = true
}

// 确认新建授权
async function handleConfirmCreateAuth() {
  try {
    // 表单验证
    if (!createAuthForm.value.authId.trim()) {
      console.warn('请填写授权ID')
      return
    }
    if (!createAuthForm.value.authType) {
      console.warn('请选择授权类型')
      return
    }
    if (!createAuthForm.value.startDate) {
      console.warn('请选择授权开始日期')
      return
    }
    if (!createAuthForm.value.endDate) {
      console.warn('请选择授权结束日期')
      return
    }
    if (!createAuthForm.value.authNote.trim()) {
      console.warn('请填写授权备注')
      return
    }

    // 日期验证
    if (new Date(createAuthForm.value.startDate) >= new Date(createAuthForm.value.endDate)) {
      console.warn('授权结束日期必须大于开始日期')
      return
    }

    // 调用新建授权API
    const res = await deliveryApi.createAuth({
      clientName: clientName_.value,
      shellNumber: createAuthShellNumber.value,
      authId: createAuthForm.value.authId,
      authType: createAuthForm.value.authType,
      startDate: createAuthForm.value.startDate,
      endDate: createAuthForm.value.endDate,
      authNote: createAuthForm.value.authNote,
    })

    if (res?.data?.success) {
      // 关闭弹窗
      createAuthDialogVisible.value = false

      // 显示成功提示
      ElMessage.success(res.data.message || '新建授权成功')

      // 刷新页面数据
      if (clientName_.value) {
        await fetchAuthDetail(clientName_.value)
      }
    }
    else {
      ElMessage.error(res?.data?.message || '新建授权失败')
    }
  }
  catch (error) {
    console.error('新建授权失败:', error)
    ElMessage.error('新建授权失败')
  }
}

// 取消新建授权
function handleCancelCreateAuth() {
  createAuthDialogVisible.value = false
  createAuthShellNumber.value = ''
  createAuthForm.value = {
    authId: '',
    authType: '',
    startDate: '',
    endDate: '',
    authNote: '',
  }
}

// ------------批量更新授权弹窗---------------
// 批量更新授权弹窗相关状态
const batchUpdateDialogVisible = ref(false)
interface UpdateAuthItem {
  shellNumber: string
  authList: Array<{
    authId: string
    startDate: string
    endDate: string
    originalEndDate: string // 保存原始结束日期用于比较
  }>
}

const updateAuthList = ref<UpdateAuthItem[]>([])

// 批量更新授权处理函数
async function handleBatchUpdateAuth() {
  if (!authDetail.value) {
    return
  }

  // 先确保所有外壳号都已加载完整数据
  for (const shell of authDetail.value.shellNumbers) {
    if (!shell.isFullyLoaded) {
      await loadFullAuthList(shell.shellNumber)
    }
  }

  // 构建更新数据结构
  updateAuthList.value = authDetail.value.shellNumbers.map(shell => ({
    shellNumber: shell.shellNumber,
    authList: shell.authorizationList.map(auth => ({
      authId: auth.authId,
      startDate: auth.startDate,
      endDate: auth.endDate,
      originalEndDate: auth.endDate,
    })),
  }))

  batchUpdateDialogVisible.value = true
}
// 确认批量更新授权
async function handleConfirmBatchUpdate() {
  try {
    // 收集所有有变更的授权记录
    const changedAuths: Array<{
      shellNumber: string
      authId: string
      newEndDate: string
    }> = []

    updateAuthList.value.forEach((shell) => {
      shell.authList.forEach((auth) => {
        if (auth.endDate !== auth.originalEndDate) {
          changedAuths.push({
            shellNumber: shell.shellNumber,
            authId: auth.authId,
            newEndDate: auth.endDate,
          })
        }
      })
    })

    if (changedAuths.length === 0) {
      ElMessage.warning('没有检测到任何变更，无法进行批量更新')
      return
    }

    // 日期验证
    for (const changed of changedAuths) {
      const shell = updateAuthList.value.find(s => s.shellNumber === changed.shellNumber)
      const auth = shell?.authList.find(a => a.authId === changed.authId)
      if (auth && new Date(auth.startDate) >= new Date(auth.endDate)) {
        ElMessage.warning(`${changed.authId}: 授权结束日期必须大于开始日期`)
        return
      }
    }

    // 调用批量更新API
    const res = await deliveryApi.batchUpdateAuthEndDates({
      clientName: clientName_.value,
      changes: changedAuths,
    })

    if (res?.data?.success) {
      // 更新本地数据
      if (authDetail.value) {
        updateAuthList.value.forEach((shellUpdate) => {
          const shell = authDetail.value!.shellNumbers.find(s => s.shellNumber === shellUpdate.shellNumber)
          if (shell) {
            shellUpdate.authList.forEach((authUpdate) => {
              const auth = shell.authorizationList.find(a => a.authId === authUpdate.authId)
              if (auth) {
                auth.endDate = authUpdate.endDate
                auth.status = calculateAuthStatus(authUpdate.endDate)
              }
            })
          }
        })
      }

      // 关闭弹窗
      batchUpdateDialogVisible.value = false

      // 显示成功提示
      ElMessage.success(res.data.message || '批量更新授权成功')

      // 刷新页面数据
      if (clientName_.value) {
        await fetchAuthDetail(clientName_.value)
      }
    }
  }
  catch (error) {
    ElMessage.error('批量更新授权失败')
    console.error('批量更新授权失败:', error)
  }
}

// 复位外壳号授权处理函数
function handleResetShellAuth(shellNumber: string) {
  const shell = updateAuthList.value.find(s => s.shellNumber === shellNumber)
  if (shell) {
    // 将该外壳号下的所有授权截止日期恢复为原始值
    shell.authList.forEach((auth) => {
      auth.endDate = auth.originalEndDate
    })

    console.warn(`已复位外壳号 ${shellNumber} 的所有授权修改`)
    ElMessage.success(`已复位外壳号 ${shellNumber} 的所有授权修改`)
  }
}

// 取消批量更新授权
function handleCancelBatchUpdate() {
  batchUpdateDialogVisible.value = false
  updateAuthList.value = []
}

// 获取批量更新时的剩余天数
function getBatchUpdateRemainingDays(endDate: string): string {
  return calculateRemainingDays(endDate)
}

// 获取批量更新时的剩余天数颜色
function getBatchUpdateRemainingDaysColor(endDate: string): string {
  return getRemainingDaysColor(endDate)
}

// --------------页面加载事件处理函数-----------------------
// 添加懒加载状态管理
const shellLoadingMap = ref<Record<string, boolean>>({}) // 各外壳号的加载状态

// 加载完整的授权列表
async function loadFullAuthList(shellNumber: string) {
  if (shellLoadingMap.value[shellNumber]) {
    return // 正在加载中，避免重复请求
  }

  shellLoadingMap.value[shellNumber] = true

  try {
    // 调用API获取该外壳号的完整授权列表
    const res = await deliveryApi.getShellAuthList({
      clientName: clientName_.value,
      shellNumber,
    })

    if (res?.data?.authorizationList && authDetail.value) {
      // 找到对应的外壳号并更新其授权列表
      const shell = authDetail.value.shellNumbers.find(s => s.shellNumber === shellNumber)
      if (shell) {
        shell.authorizationList = res.data.authorizationList.map((auth: any) => ({
          authId: auth.authId || '', // 授权ID
          startDate: auth.startDate || '', // 授权开始日期 (YYYY-MM-DD)
          endDate: auth.endDate || '', // 授权结束日期 (YYYY-MM-DD)
          authType: auth.authType || '', // 授权类型 本地锁/网络锁/软锁授权
          authNote: auth.authNote || '', // 授权备注
          status: calculateAuthStatus(auth.endDate || ''), // 授权状态（有效、临期、过期）前端计算
        }))
        shell.isFullyLoaded = true
      }
    }
  }
  catch (error) {
    console.error(`加载外壳号 ${shellNumber} 完整授权列表失败:`, error)
    ElMessage.error(`加载外壳号 ${shellNumber} 完整授权列表失败`)
  }
  finally {
    shellLoadingMap.value[shellNumber] = false
  }
}

// 获取授权详情数据
async function fetchAuthDetail(clientName: string) {
  loading.value = true

  try {
    // 调用API获取概览数据（每个外壳号只返回第一条授权记录）
    const res = await deliveryApi.getAuthDetail(clientName)

    if (res?.data) {
      const mappedData: AuthDetail = {
        clientName: res.data.clientName || clientName, // 客户名称
        dongleCount: res.data.dongleCount || 0, // 加密狗数量
        shellNumbers: (res.data.shellNumbers || []).map((shell: any) => ({
          shellNumber: shell.shellNumber || '', // 外壳号
          deviceType: shell.deviceType || '', // 客户设备类型 lab/IPT/FTD/FFS
          deviceNote: shell.deviceNote || '', // 客户设备备注
          contractName: shell.contractName || '', // 合同名称
          contractNumber: shell.contractNumber || '', // 合同编号
          authCount: shell.authCount || 0, // 该外壳号的总授权数量
          outTime: shell.outTime || 0, // 后端获取外壳号出库时间
          isFullyLoaded: false, // 初始状态为未完全加载
          // 修改这里：不要使用 slice(0, 1)，因为 Mock 数据已经只返回第一条
          authorizationList: (shell.authorizationList || []).map((auth: any) => ({
            authId: auth.authId || '', // 授权ID
            startDate: auth.startDate || '', // 授权开始日期 (YYYY-MM-DD)
            endDate: auth.endDate || '', // 授权结束日期 (YYYY-MM-DD)
            authType: auth.authType || '', // 授权类型 本地锁/网络锁/软锁授权
            authNote: auth.authNote || '', // 授权备注
            status: calculateAuthStatus(auth.endDate || ''), // 授权状态（有效、临期、过期）前端计算
          })),
        })),
      }

      authDetail.value = mappedData

      // 初始化展开状态和加载状态
      expandedMap.value = {}
      shellLoadingMap.value = {}
      mappedData.shellNumbers.forEach((shell) => {
        expandedMap.value[shell.shellNumber] = false
        shellLoadingMap.value[shell.shellNumber] = false
      })

      console.warn('获取授权详情成功')
    }
  }
  catch (error) {
    console.error('获取授权详情失败:', error)
    ElMessage.error('获取授权详情失败，请稍后重试')
  }
  finally {
    loading.value = false
  }
}

// 页面加载时获取客户名称并请求授权详情
onMounted(() => {
  clientName_.value = route.query.clientName as string || '' // 获取客户名称
  dongleCount_.value = route.query.dongleCount as string || '' // 获取客户加密狗数量
  validCount_.value = route.query.validCount as string || '' // 获取客户有效授权数量
  expiringCount_.value = route.query.expiringCount as string || '' // 获取客户临期授权数量
  expiredCount_.value = route.query.expiredCount as string || '' // 获取客户过期授权数量

  // 转换为数字进行相加
  const validNum = Number(validCount_.value) || 0
  const expiringNum = Number(expiringCount_.value) || 0
  const expiredNum = Number(expiredCount_.value) || 0
  totalAuthCount_.value = String(validNum + expiringNum + expiredNum) // 计算客户总授权数量

  if (clientName_.value) {
    fetchAuthDetail(clientName_.value)
  }
})

// 返回客户管理页面
function handleBackToClientManage() {
  console.warn('返回客户管理页面')
  router.replace({ path: '/client_manage' })
}
</script>

<template>
  <div>
    <el-skeleton :loading="loading" animated>
      <template #default>
        <!-- 顶部标题栏和统计信息 -->
        <div class="mb-6 border border-gray-200 rounded-lg bg-white p-6 shadow-sm">
          <!-- 第一行：标题和功能按钮 -->
          <div class="mb-4 flex items-center justify-between">
            <div class="flex items-center">
              <h1 class="text-3xl text-gray-800 font-bold">
                {{ clientName_ }} - 授权详情
              </h1>
            </div>
            <!-- 右边：返回按钮 -->
            <div class="flex items-center gap-3">
              <FaButton
                class="border border-gray-500 rounded bg-gray-300 px-4 py-2 text-sm text-black font-semibold transition-colors duration-150 hover:bg-gray-400"
                @click="handleBackToClientManage"
              >
                返回客户管理
              </FaButton>
            </div>
          </div>

          <!-- 第二行：统计信息和功能按钮 -->
          <div v-if="authDetail" class="flex items-center justify-between">
            <!-- 左边：统计信息 -->
            <div class="flex items-center gap-8">
              <div class="flex items-center gap-3">
                <span class="text-sm text-gray-600 font-bold">加密狗数量：</span>
                <span class="text-xl text-blue-600 font-bold">{{ dongleCount_ }}</span>
              </div>
              <div class="flex items-center gap-3">
                <span class="text-sm text-gray-600 font-bold">授权ID（有效/临期/过期/总数）：</span>
                <span class="text-xl font-bold">
                  <span class="text-green-600">{{ validCount_ }}</span>
                  <span class="text-gray-400">/</span>
                  <span class="text-yellow-600">{{ expiringCount_ }}</span>
                  <span class="text-gray-400">/</span>
                  <span class="text-gray-500">{{ expiredCount_ }}</span>
                  <span class="text-gray-400">/</span>
                  <span class="text-black">{{ totalAuthCount_ }}</span>
                </span>
              </div>
            </div>

            <!-- 右边：功能按钮 -->
            <div class="flex items-center gap-3">
              <FaButton
                class="border border-green-500 rounded bg-green-300 px-5 py-2 text-black font-semibold transition-colors duration-150 hover:bg-green-400"
                @click="handleDeliverShell"
              >
                交付外壳
              </FaButton>
              <FaButton
                class="border border-red-500 rounded bg-red-300 px-5 py-2 text-black font-semibold transition-colors duration-150 hover:bg-red-400"
                @click="handleReturnShell"
              >
                归还外壳
              </FaButton>
              <FaButton
                class="border border-purple-500 rounded bg-purple-300 px-5 py-2 text-black font-semibold transition-colors duration-150 hover:bg-purple-400"
                @click="handleBatchUpdateAuth"
              >
                更新授权
              </FaButton>
            </div>
          </div>
        </div>

        <!-- 外壳号列表 -->
        <div v-if="authDetail">
          <FaPageMain
            v-for="shell in authDetail.shellNumbers"
            :key="shell.shellNumber"
            :title="shell.shellNumber"
            height="auto"
            class="mb-4 w-full"
          >
            <template #title>
              <div class="w-full">
                <!-- 标题行 -->
                <div class="flex items-center justify-between">
                  <div class="flex items-center gap-3">
                    <!-- 外壳号状态圆点 -->
                    <div
                      class="h-4 w-4 flex-shrink-0 rounded-full"
                      :class="getShellStatusDotColor(shell)"
                    />
                    <span class="text-xl text-black font-bold">
                      {{ shell.shellNumber }}
                    </span>
                  </div>
                  <!-- 右上角按钮组 -->
                  <div class="mr-4 flex items-center gap-2">
                    <FaButton
                      class="border border-green-500 rounded bg-green-300 px-3 py-1 text-sm text-black font-semibold transition-colors duration-150 hover:bg-green-400"
                      @click.stop="handleCreateAuthForShell(shell.shellNumber)"
                    >
                      新建授权
                    </FaButton>
                    <FaButton
                      class="border border-blue-500 rounded bg-blue-300 px-3 py-1 text-sm text-black font-semibold transition-colors duration-150 hover:bg-blue-400"
                      @click.stop="handleEditShell(shell.shellNumber)"
                    >
                      编辑
                    </FaButton>
                  </div>
                </div>
                <!-- 设备类型和备注 -->
                <div class="ml-2 mt-2 flex items-center gap-6">
                  <span class="text-base text-black">
                    <span class="font-bold">设备类型：</span>{{ shell.deviceType }}
                  </span>
                  <span class="text-base text-black">
                    <span class="font-bold">设备备注：</span>{{ shell.deviceNote }}
                  </span>
                  <span v-if="shell.contractName" class="text-base text-black">
                    <span class="font-bold">合同名称：</span>{{ shell.contractName }}
                  </span>
                  <span v-if="shell.contractNumber" class="text-base text-black">
                    <span class="font-bold">合同编号：</span>{{ shell.contractNumber }}
                  </span>
                </div>
              </div>
            </template>

            <!-- 外壳号内容区域：始终展示所有授权信息 -->
            <div class="space-y-4">
              <div v-if="shell.authorizationList.length === 0" class="py-6 text-center text-gray-400">
                暂无授权信息
              </div>
              <el-table
                v-else
                :data="shell.authorizationList"
                border
                style="width: 100%;"
                size="small"
                class="bg-white"
              >
                <el-table-column prop="authId" label="授权ID" min-width="120">
                  <template #default="scope">
                    <div class="flex items-center gap-2">
                      <span class="text-black font-bold">{{ scope.row.authId }}</span>
                    </div>
                  </template>
                </el-table-column>
                <el-table-column prop="startDate" label="开始日期" min-width="110">
                  <template #default="scope">
                    <span class="text-gray-700 font-bold">{{ scope.row.startDate }}</span>
                  </template>
                </el-table-column>
                <el-table-column prop="endDate" label="结束日期" min-width="110">
                  <template #default="scope">
                    <span class="text-gray-700 font-bold">{{ scope.row.endDate }}</span>
                  </template>
                </el-table-column>
                <el-table-column prop="authType" label="授权类型" min-width="100">
                  <template #default="scope">
                    <span class="text-gray-700 font-bold">{{ scope.row.authType }}</span>
                  </template>
                </el-table-column>
                <el-table-column label="剩余天数" min-width="100">
                  <template #default="scope">
                    <span :class="getRemainingDaysColor(scope.row.endDate)" class="font-bold">
                      {{ calculateRemainingDays(scope.row.endDate) }}
                    </span>
                  </template>
                </el-table-column>
                <el-table-column
                  prop="authNote"
                  label="授权备注"
                  min-width="180"
                  show-overflow-tooltip
                >
                  <template #default="scope">
                    <span class="text-gray-700 font-bold">{{ scope.row.authNote }}</span>
                  </template>
                </el-table-column>
              </el-table>
            </div>
          </FaPageMain>
        </div>

        <!-- 交付外壳弹窗 -->
        <el-dialog
          v-model="deliverShellDialogVisible"
          title="交付外壳"
          width="50vw"
          :close-on-click-modal="false"
          :close-on-press-escape="false"
        >
          <template #header>
            <div class="w-full flex items-center justify-between">
              <div class="flex items-center gap-2">
                <el-icon class="text-xl text-green-600">
                  <Box />
                </el-icon>
                <span class="text-lg font-bold">交付外壳</span>
              </div>
              <span class="text-base text-gray-600 font-bold">客户：{{ clientName_ }}</span>
            </div>
          </template>

          <div class="space-y-5">
            <!-- 提示信息 -->
            <div class="border border-blue-200 rounded-lg bg-blue-50 p-4">
              <div class="flex items-start gap-3">
                <el-icon class="mt-0.5 text-blue-600">
                  <InfoFilled />
                </el-icon>
                <div class="flex-1">
                  <div class="mb-1 text-sm text-blue-800 font-semibold">
                    交付外壳说明
                  </div>
                  <div class="text-sm text-blue-700 leading-relaxed">
                    • 从库存中选择可交付的外壳号<br>
                    • 交付后该外壳号将分配给当前客户<br>
                    • 可在该外壳号下创建授权信息
                  </div>
                </div>
              </div>
            </div>

            <div class="flex items-center gap-3">
              <label class="w-24 text-sm text-gray-600 font-bold">
                <span class="text-red-500">*</span>
                外壳号：
              </label>
              <el-select
                v-model="deliverShellForm.shellNumber"
                placeholder="请选择要交付的外壳号"
                class="flex-1"
                :loading="shellsLoading"
                filterable
              >
                <template #prefix>
                  <el-icon><Cpu /></el-icon>
                </template>
                <!-- 简化选项结构 -->
                <el-option
                  v-for="shellNumber in availableShells"
                  :key="shellNumber"
                  :label="shellNumber"
                  :value="shellNumber"
                >
                  <div class="flex items-center gap-2">
                    <el-icon><Cpu /></el-icon>
                    <span class="font-medium">{{ shellNumber }}</span>
                  </div>
                </el-option>
                <template #empty>
                  <div class="py-4 text-center">
                    <div v-if="shellsLoading" class="text-gray-500">
                      <el-icon class="is-loading mr-2">
                        <Loading />
                      </el-icon>
                      正在加载可交付外壳号...
                    </div>
                    <div v-else class="text-gray-500">
                      暂无可交付的外壳号
                    </div>
                  </div>
                </template>
              </el-select>
            </div>

            <!-- 设备类型选择 -->
            <div class="flex items-center gap-3">
              <label class="w-24 text-sm text-gray-600 font-bold">
                <span class="text-red-500">*</span>
                设备类型：
              </label>
              <el-select
                v-model="deliverShellForm.deviceType"
                placeholder="请选择设备类型"
                class="flex-1"
              >
                <template #prefix>
                  <el-icon><Monitor /></el-icon>
                </template>
                <el-option
                  v-for="option in deviceTypeOptions"
                  :key="option.value"
                  :label="option.label"
                  :value="option.value"
                >
                  <div class="flex items-center gap-2">
                    <el-icon><Monitor /></el-icon>
                    <span>{{ option.label }}</span>
                  </div>
                </el-option>
              </el-select>
            </div>
            <!-- 合同名称 -->
            <div class="flex items-start gap-3">
              <label class="mt-2 w-24 flex-shrink-0 text-sm text-gray-600 font-bold">
                <!-- <span class="text-red-500">*</span> -->
                合同名称：
              </label>
              <el-input
                v-model="deliverShellForm.contractName"
                type="text"
                placeholder="请输入合同名称"
                maxlength="200"
                show-word-limit
                resize="none"
                class="flex-1"
              />
            </div>
            <!-- 合同编号 -->
            <div class="flex items-start gap-3">
              <label class="mt-2 w-24 flex-shrink-0 text-sm text-gray-600 font-bold">
                <!-- <span class="text-red-500">*</span> -->
                合同编号：
              </label>
              <el-input
                v-model="deliverShellForm.contractNumber"
                type="text"
                placeholder="请输入合同编号"
                maxlength="200"
                show-word-limit
                resize="none"
                class="flex-1"
              />
            </div>

            <!-- 设备备注输入 -->
            <div class="flex items-start gap-3">
              <label class="mt-2 w-24 flex-shrink-0 text-sm text-gray-600 font-bold">
                <span class="text-red-500">*</span>
                设备备注：
              </label>
              <el-input
                v-model="deliverShellForm.deviceNote"
                type="textarea"
                placeholder="请输入设备备注信息，如：设备用途、部署位置等"
                :rows="4"
                maxlength="200"
                show-word-limit
                resize="none"
                class="flex-1"
              />
            </div>

            <!-- 预览信息 -->
            <div
              v-if="deliverShellForm.shellNumber || deliverShellForm.deviceType || deliverShellForm.deviceNote"
              class="border border-gray-200 rounded-lg bg-gray-50 p-4"
            >
              <div class="mb-2 text-sm text-gray-800 font-semibold">
                交付预览：
              </div>
              <div class="text-sm text-gray-700 space-y-1">
                <div v-if="deliverShellForm.shellNumber">
                  <span class="font-medium">外壳号：</span>{{ deliverShellForm.shellNumber }}
                </div>
                <div v-if="deliverShellForm.deviceType">
                  <span class="font-medium">设备类型：</span>{{ deliverShellForm.deviceType }}
                </div>
                <div v-if="deliverShellForm.deviceNote">
                  <span class="font-medium">设备备注：</span>{{ deliverShellForm.deviceNote }}
                </div>
                <div v-if="deliverShellForm.contractName">
                  <span class="font-medium">合同名称：</span>{{ deliverShellForm.contractName }}
                </div>
                <div v-if="deliverShellForm.contractNumber">
                  <span class="font-medium">合同编号：</span>{{ deliverShellForm.contractNumber }}
                </div>
              </div>
            </div>
          </div>

          <template #footer>
            <div class="flex justify-end gap-3">
              <el-button @click="handleCancelDeliverShell">
                取消
              </el-button>
              <el-button
                type="primary"
                :disabled="!deliverShellForm.shellNumber.trim() || !deliverShellForm.deviceType || !deliverShellForm.deviceNote.trim()"
                @click="handleConfirmDeliverShell"
              >
                确认交付
              </el-button>
            </div>
          </template>
        </el-dialog>

        <!-- 归还外壳弹窗 -->
        <el-dialog
          v-model="returnShellDialogVisible"
          :title="getOperationTitle || '外壳操作'"
          width="50vw"
          :close-on-click-modal="false"
          :close-on-press-escape="false"
        >
          <template #header>
            <div class="w-full flex items-center justify-between">
              <div class="flex items-center gap-2">
                <el-icon
                  class="text-xl" :class="{
                    'text-red-600': returnShellForm.operationType === '入库',
                    'text-orange-600': returnShellForm.operationType === '丢失',
                    'text-purple-600': returnShellForm.operationType === '损坏',
                    'text-gray-600': !returnShellForm.operationType,
                  }"
                >
                  <component
                    :is="returnShellForm.operationType === 'return' ? 'Upload'
                      : returnShellForm.operationType === 'lost' ? 'Warning'
                        : returnShellForm.operationType === 'damaged' ? 'Tools' : 'Operation'"
                  />
                </el-icon>
                <span class="text-lg font-bold">{{ getOperationTitle }}</span>
              </div>
              <span class="text-base text-gray-600 font-bold">客户：{{ clientName_ }}</span>
            </div>
          </template>

          <div class="space-y-5">
            <!-- 操作类型选择 -->
            <div class="flex items-center gap-3">
              <label class="w-24 text-sm text-gray-600 font-bold">
                <span class="text-red-500">*</span>
                操作类型：
              </label>
              <el-select
                v-model="returnShellForm.operationType"
                placeholder="请选择操作类型"
                class="flex-1"
                @change="handleOperationTypeChange"
              >
                <template #prefix>
                  <el-icon><Operation /></el-icon>
                </template>
                <el-option
                  v-for="option in operationTypeOptions"
                  :key="option.value"
                  :label="option.label"
                  :value="option.value"
                >
                  <div class="flex flex-col">
                    <span class="font-medium">{{ option.label }}</span>
                    <span class="text-xs text-gray-500">{{ option.description }}</span>
                  </div>
                </el-option>
              </el-select>
            </div>

            <!-- 提示信息 - 根据操作类型动态显示 -->
            <div
              v-if="returnShellForm.operationType" class="border rounded-lg p-4" :class="{
                'border-amber-200 bg-amber-50': returnShellForm.operationType === 'return',
                'border-orange-200 bg-orange-50': returnShellForm.operationType === 'lost',
                'border-purple-200 bg-purple-50': returnShellForm.operationType === 'damaged',
              }"
            >
              <div class="flex items-start gap-3">
                <el-icon
                  class="mt-0.5" :class="{
                    'text-amber-600': returnShellForm.operationType === 'return',
                    'text-orange-600': returnShellForm.operationType === 'lost',
                    'text-purple-600': returnShellForm.operationType === 'damaged',
                  }"
                >
                  <component
                    :is="returnShellForm.operationType === 'return' ? 'Warning'
                      : returnShellForm.operationType === 'lost' ? 'CircleClose' : 'Tools'"
                  />
                </el-icon>
                <div class="flex-1">
                  <div
                    class="mb-1 text-sm font-semibold" :class="{
                      'text-amber-800': returnShellForm.operationType === 'return',
                      'text-orange-800': returnShellForm.operationType === 'lost',
                      'text-purple-800': returnShellForm.operationType === 'damaged',
                    }"
                  >
                    {{ getOperationTitle }}说明
                  </div>
                  <div
                    class="text-sm leading-relaxed" :class="{
                      'text-amber-700': returnShellForm.operationType === 'return',
                      'text-orange-700': returnShellForm.operationType === 'lost',
                      'text-purple-700': returnShellForm.operationType === 'damaged',
                    }" v-html="getOperationDescription"
                  />
                </div>
              </div>
            </div>

            <!-- 外壳号选择 -->
            <div v-if="returnShellForm.operationType" class="flex items-center gap-3">
              <label class="w-24 text-sm text-gray-600 font-bold">
                <span class="text-red-500">*</span>
                外壳号：
              </label>
              <el-select
                v-model="returnShellForm.shellNumber"
                placeholder="请选择要操作的外壳号"
                class="flex-1"
                @change="handleShellNumberChange"
              >
                <template #prefix>
                  <el-icon><Cpu /></el-icon>
                </template>
                <el-option
                  v-for="shell in getReturnableShells"
                  :key="shell.value"
                  :label="shell.label"
                  :value="shell.value"
                >
                  <div class="w-full flex items-center justify-between">
                    <div class="flex items-center gap-2">
                      <el-icon><Cpu /></el-icon>
                      <span class="font-medium">{{ shell.value }}</span>
                    </div>
                    <div class="text-sm text-gray-500">
                      {{ shell.deviceType }} | {{ shell.authCount }}个授权
                    </div>
                  </div>
                </el-option>
              </el-select>
            </div>

            <!-- 时间选择 -->
            <div v-if="returnShellForm.operationType" class="flex items-center gap-3">
              <label class="w-24 text-sm text-gray-600 font-bold">
                <span class="text-red-500">*</span>
                {{ getDateLabel }}：
              </label>

              <!-- 归还外壳：显示当前时间（只读） -->
              <el-input
                v-if="returnShellForm.operationType === 'return'"
                :value="returnShellForm.returnDate"
                readonly
                class="flex-1"
                placeholder="自动设置为当前时间"
              >
                <template #prefix>
                  <el-icon><Clock /></el-icon>
                </template>
              </el-input>

              <!-- 丢失/损坏：用户选择日期时间 -->
              <el-date-picker
                v-else
                v-model="returnShellForm.returnDate"
                type="datetime"
                :placeholder="`请选择${getDateLabel}`"
                format="YYYY-MM-DD HH:mm:ss"
                value-format="YYYY-MM-DD HH:mm:ss"
                :disabled-date="(date: Date) => date > new Date()"
                class="flex-1"
              >
                <template #prefix>
                  <el-icon><Calendar /></el-icon>
                </template>
              </el-date-picker>
            </div>

            <!-- 外壳号备注 - 仅在丢失和损坏时显示 -->
            <div v-if="shouldShowRemark" class="flex items-start gap-3">
              <label class="mt-2 w-24 flex-shrink-0 text-sm text-gray-600 font-bold">
                <span class="text-red-500">*</span>
                外壳备注：
              </label>
              <el-input
                v-model="returnShellForm.remark"
                type="textarea"
                :placeholder="returnShellForm.operationType === 'lost'
                  ? '请详细描述外壳丢失的情况、可能原因等...'
                  : '请详细描述外壳损坏的情况、损坏程度、处理建议等...'"
                :rows="4"
                maxlength="500"
                show-word-limit
                resize="none"
                class="flex-1"
              />
            </div>

            <!-- 外壳号详细信息显示 -->
            <div v-if="returnShellForm.shellNumber" class="border border-gray-200 rounded-lg bg-gray-50 p-4">
              <div class="mb-2 text-sm text-gray-800 font-semibold">
                外壳号详细信息：
              </div>
              <div class="text-sm text-gray-700 space-y-1">
                <div v-for="shell in getReturnableShells" :key="shell.value">
                  <template v-if="shell.value === returnShellForm.shellNumber">
                    <div><span class="font-medium">外壳号：</span>{{ shell.value }}</div>
                    <div><span class="font-medium">设备类型：</span>{{ shell.deviceType }}</div>
                    <div><span class="font-medium">授权数量：</span>{{ shell.authCount }}个</div>
                    <div><span class="font-medium">设备备注：</span>{{ shell.deviceNote }}</div>
                    <div><span class="font-medium">合同名称：</span>{{ shell.contractName }}</div>
                    <div><span class="font-medium">合同编号：</span>{{ shell.contractNumber }}</div>
                  </template>
                </div>
              </div>
            </div>

            <!-- 预览信息 -->
            <div
              v-if="returnShellForm.operationType && returnShellForm.shellNumber && returnShellForm.returnDate"
              class="border border-blue-200 rounded-lg bg-blue-50 p-4"
            >
              <div class="mb-2 text-sm text-blue-800 font-semibold">
                操作预览：
              </div>
              <div class="text-sm text-blue-700 space-y-1">
                <div><span class="font-medium">操作类型：</span>{{ getOperationTitle }}</div>
                <div><span class="font-medium">外壳号：</span>{{ returnShellForm.shellNumber }}</div>
                <div>
                  <span class="font-medium">{{ getDateLabel }}：</span>
                  {{ returnShellForm.returnDate }}
                  <span v-if="returnShellForm.operationType === 'return'" class="text-blue-600">
                    (当前时间)
                  </span>
                </div>
                <div v-if="shouldShowRemark && returnShellForm.remark">
                  <span class="font-medium">备注：</span>{{ returnShellForm.remark }}
                </div>
              </div>
            </div>
          </div>

          <template #footer>
            <div class="flex justify-end gap-3">
              <el-button @click="handleCancelReturnShell">
                取消
              </el-button>
              <el-button
                :type="returnShellForm.operationType === 'return' ? 'primary' : 'warning'"
                :disabled="!returnShellForm.operationType || !returnShellForm.shellNumber || !returnShellForm.returnDate || (shouldShowRemark && !returnShellForm.remark.trim())"
                @click="handleConfirmReturnShell"
              >
                确认{{ getOperationTitle }}
              </el-button>
            </div>
          </template>
        </el-dialog>

        <!-- 编辑外壳号信息弹窗 -->
        <el-dialog
          v-model="editDialogVisible"
          title="编辑外壳号信息"
          width="50vw"
          :close-on-click-modal="false"
        >
          <template #header>
            <div class="w-full flex items-center justify-between">
              <span class="text-lg font-bold">编辑外壳号信息</span>
              <span class="text-base text-gray-600 font-bold">客户：{{ clientName_ }}</span>
            </div>
          </template>
          <div v-if="editingShell" class="space-y-4">
            <!-- 外壳号显示（只读） -->
            <div class="flex items-center gap-3">
              <label class="w-20 text-sm text-gray-600 font-bold">外壳号：</label>
              <el-input
                :value="editingShell.shellNumber"
                readonly
                class="flex-1"
              />
            </div>

            <!-- 设备类型选择 -->
            <div class="flex items-center gap-3">
              <label class="w-20 text-sm text-gray-600 font-bold">设备类型：</label>
              <el-select
                v-model="editForm.deviceType"
                placeholder="请选择设备类型"
                class="flex-1"
              >
                <el-option
                  v-for="option in deviceTypeOptions"
                  :key="option.value"
                  :label="option.label"
                  :value="option.value"
                />
              </el-select>
            </div>
            <!-- 合同名称输入 -->
            <div class="flex items-start gap-3">
              <label class="mt-2 w-24 flex-shrink-0 text-sm text-gray-600 font-bold">合同名称：</label>
              <el-input
                v-model="editForm.contractName"
                placeholder="请输入合同名称"
                class="flex-1"
              />
            </div>
            <!-- 合同编号输入 -->
            <div class="flex items-start gap-3">
              <label class="mt-2 w-24 flex-shrink-0 text-sm text-gray-600 font-bold">合同编号：</label>
              <el-input
                v-model="editForm.contractNumber"
                placeholder="请输入合同编号"
                class="flex-1"
              />
            </div>

            <!-- 设备备注输入 -->
            <div class="flex items-start gap-3">
              <label class="mt-2 w-20 flex-shrink-0 text-sm text-gray-600 font-bold">设备备注：</label>
              <el-input
                v-model="editForm.deviceNote"
                type="textarea"
                placeholder="请输入设备备注"
                :rows="3"
                maxlength="200"
                show-word-limit
                class="flex-1"
              />
            </div>
          </div>

          <template #footer>
            <div class="flex justify-end gap-3">
              <el-button @click="handleCancelEdit">
                取消
              </el-button>
              <el-button
                type="primary"
                :disabled="!editForm.deviceType.trim() || !editForm.deviceNote.trim()"
                @click="handleConfirmEdit"
              >
                确认
              </el-button>
            </div>
          </template>
        </el-dialog>

        <!-- 新建授权信息弹窗 -->
        <el-dialog
          v-model="createAuthDialogVisible"
          title="新建授权信息"
          width="50vw"
          :close-on-click-modal="false"
        >
          <template #header>
            <div class="w-full flex items-center justify-between">
              <span class="text-lg font-bold">新建授权信息</span>
              <span class="text-base text-gray-600 font-bold">客户：{{ clientName_ }}</span>
            </div>
          </template>
          <div class="space-y-4">
            <!-- 外壳号显示（只读） -->
            <div class="flex items-center gap-3">
              <label class="w-24 text-sm text-gray-600 font-bold">外壳号：</label>
              <el-input
                :value="createAuthShellNumber"
                readonly
                class="flex-1"
              />
            </div>

            <!-- 授权ID输入 -->
            <div class="flex items-center gap-3">
              <label class="w-24 text-sm text-gray-600 font-bold">授权ID：</label>
              <el-input
                v-model="createAuthForm.authId"
                placeholder="请输入授权ID，十位纯数字"
                maxlength="50"
                class="flex-1"
              />
            </div>

            <!-- 授权类型选择 -->
            <div class="flex items-center gap-3">
              <label class="w-24 text-sm text-gray-600 font-bold">授权类型：</label>
              <el-select
                v-model="createAuthForm.authType"
                placeholder="请选择授权类型"
                class="flex-1"
              >
                <el-option
                  v-for="option in authTypeOptions"
                  :key="option.value"
                  :label="option.label"
                  :value="option.value"
                />
              </el-select>
            </div>

            <!-- 授权开始日期 -->
            <div class="flex items-center gap-3">
              <label class="w-24 text-sm text-gray-600 font-bold">开始日期：</label>
              <el-date-picker
                v-model="createAuthForm.startDate"
                type="date"
                placeholder="请选择授权开始日期"
                format="YYYY-MM-DD"
                value-format="YYYY-MM-DD"
                class="flex-1"
              />
            </div>

            <!-- 授权结束日期 -->
            <div class="flex items-center gap-3">
              <label class="w-24 text-sm text-gray-600 font-bold">结束日期：</label>
              <el-date-picker
                v-model="createAuthForm.endDate"
                type="date"
                placeholder="请选择授权结束日期"
                format="YYYY-MM-DD"
                value-format="YYYY-MM-DD"
                :disabled-date="(date: Date) => createAuthForm.startDate ? date <= new Date(createAuthForm.startDate) : false"
                class="flex-1"
              />
            </div>

            <!-- 授权备注输入 -->
            <div class="flex items-start gap-3">
              <label class="mt-2 w-24 flex-shrink-0 text-sm text-gray-600 font-bold">授权备注：</label>
              <el-input
                v-model="createAuthForm.authNote"
                type="textarea"
                placeholder="请输入授权备注信息"
                :rows="4"
                maxlength="500"
                show-word-limit
                class="flex-1"
              />
            </div>
          </div>

          <template #footer>
            <div class="flex justify-end gap-3">
              <el-button @click="handleCancelCreateAuth">
                取消
              </el-button>
              <el-button
                type="primary"
                :disabled="!createAuthForm.authId.trim() || !createAuthForm.authType || !createAuthForm.startDate || !createAuthForm.endDate || !createAuthForm.authNote.trim()"
                @click="handleConfirmCreateAuth"
              >
                确认新建
              </el-button>
            </div>
          </template>
        </el-dialog>

        <!-- 批量更新授权弹窗 -->
        <el-dialog
          v-model="batchUpdateDialogVisible"
          title="批量更新授权"
          width="90%"
          :close-on-click-modal="false"
          class="max-h-[80vh]"
        >
          <template #header>
            <div class="w-full flex items-center justify-between">
              <span class="text-lg font-bold">批量更新授权</span>
              <span class="text-base text-gray-600 font-bold">客户：{{ clientName_ }}</span>
            </div>
          </template>
          <div class="max-h-[60vh] overflow-y-auto space-y-6">
            <!-- 提示信息 -->
            <div class="border border-blue-200 rounded bg-blue-50 p-3">
              <div class="text-sm text-blue-700">
                <i class="el-icon-info mr-2" />
                只能修改授权截止日期，修改后的日期必须大于授权开始日期。有变更的授权会以不同颜色标识。
              </div>
            </div>

            <!-- 外壳号列表 -->
            <div
              v-for="shell in updateAuthList"
              :key="shell.shellNumber"
              class="border border-gray-200 rounded-lg bg-white shadow-sm"
            >
              <!-- 外壳号标题 -->
              <div class="border-b border-gray-200 bg-gray-50 px-4 py-3">
                <div class="flex items-center justify-between">
                  <div class="flex items-center gap-3">
                    <i class="i-mdi-chip text-xl text-black" />
                    <span class="text-lg text-black font-bold">{{ shell.shellNumber }}</span>
                    <span class="text-sm text-gray-500">({{ shell.authList.length }}个授权)</span>
                  </div>

                  <!-- 复位按钮（右边） -->
                  <div class="flex items-center">
                    <FaButton
                      class="border border-gray-600 rounded bg-gray-300 px-2 py-1 text-xs text-black font-semibold transition-colors duration-150 hover:bg-gray-400"
                      @click="handleResetShellAuth(shell.shellNumber)"
                    >
                      复位
                    </FaButton>
                  </div>
                </div>
              </div>

              <!-- 授权列表 -->
              <div class="p-4 space-y-3">
                <div
                  v-for="auth in shell.authList"
                  :key="auth.authId"
                  class="border rounded-lg p-3 transition-colors duration-200"
                  :class="auth.endDate !== auth.originalEndDate ? 'border-blue-300 bg-blue-50' : 'border-gray-200 bg-gray-50'"
                >
                  <!-- 授权信息行 -->
                  <div class="grid grid-cols-5 items-center gap-3">
                    <!-- 授权ID -->
                    <div class="flex items-center gap-1">
                      <span class="whitespace-nowrap text-sm text-gray-600 font-bold">授权ID：</span>
                      <span class="truncate text-sm text-black font-bold">{{ auth.authId }}</span>
                    </div>

                    <!-- 开始日期 -->
                    <div class="flex items-center gap-1">
                      <span class="whitespace-nowrap text-sm text-gray-600 font-bold">开始日期：</span>
                      <span class="whitespace-nowrap text-sm text-gray-700">{{ auth.startDate }}</span>
                    </div>

                    <!-- 截止日期（可编辑） -->
                    <div class="flex items-center gap-1">
                      <span class="flex-shrink-0 whitespace-nowrap text-sm text-gray-600 font-bold">截止日期：</span>
                      <el-date-picker
                        v-model="auth.endDate"
                        type="date"
                        placeholder="选择截止日期"
                        format="YYYY-MM-DD"
                        value-format="YYYY-MM-DD"
                        :disabled-date="(date: Date) => date <= new Date(auth.startDate)"
                        size="small"
                        class="w-32 flex-shrink-0"
                      />
                    </div>

                    <!-- 剩余天数 -->
                    <div class="flex items-center gap-1">
                      <span class="flex-shrink-0 whitespace-nowrap text-sm text-gray-600 font-bold">剩余天数：</span>
                      <span
                        class="whitespace-nowrap text-sm font-bold"
                        :class="getBatchUpdateRemainingDaysColor(auth.endDate)"
                      >
                        {{ getBatchUpdateRemainingDays(auth.endDate) }}
                      </span>
                    </div>

                    <!-- 变更标识 -->
                    <div class="flex flex-shrink-0 justify-end">
                      <span
                        v-if="auth.endDate !== auth.originalEndDate"
                        class="whitespace-nowrap rounded bg-blue-100 px-2 py-1 text-xs text-blue-700 font-bold"
                      >
                        已修改
                      </span>
                      <span
                        v-else
                        class="whitespace-nowrap text-xs text-gray-400"
                      >
                        未修改
                      </span>
                    </div>
                  </div>
                </div>
              </div>
            </div>
          </div>

          <template #footer>
            <div class="flex items-center justify-between">
              <!-- 统计信息 -->
              <div class="text-sm text-gray-600">
                共 {{ updateAuthList.reduce((total, shell) => total + shell.authList.length, 0) }} 个授权，
                {{ updateAuthList.reduce((total, shell) => total + shell.authList.filter(auth => auth.endDate !== auth.originalEndDate).length, 0) }} 个有变更
              </div>

              <!-- 操作按钮 -->
              <div class="flex gap-3">
                <el-button @click="handleCancelBatchUpdate">
                  取消
                </el-button>
                <el-button
                  type="primary"
                  @click="handleConfirmBatchUpdate"
                >
                  确认更新
                </el-button>
              </div>
            </div>
          </template>
        </el-dialog>
      </template>
    </el-skeleton>
  </div>
</template>
