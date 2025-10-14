<route lang="yaml">
meta:
  title: 加密管理
  # enabled: false
</route>

<script setup lang="ts">
import { ElMessage } from 'element-plus'
import { onMounted, ref } from 'vue'
import { useRouter } from 'vue-router'
import deliveryApi from '@/api/modules/delivery'

// ----------------定义数据结构--------------------
// 添加路由实例
const router = useRouter()
const loading = ref(false) // loading变量定义
// 动态数据：加密锁列表
const dongleList = ref<Dongle[]>([])

// 定义加密管理数据结构
interface Dongle {
  // -------------加密锁表-----------------
  dongleId: string // 加密锁ID
  shellCode: string // 外壳号
  shellSerial: string // 外壳序列号
  dongleRemark: string // 备注
  dongleStatus: string // 加密锁状态 出库；入库；损坏；丢失

  // -----------客户信息表-----------------
  clientName: string // 客户名称
  clientDeviceType: string // 客户设备类型 lab/IPT/FTD/FFS
  clientNote: string // 客户电脑备注

  // ----------加密锁历史信息表-------------
  inTime: string // 入库时间
  outTime: string // 出库时间
}

// 一键展开/收起功能
const expandedMap = ref<Record<string, boolean>>({})

// 获取入库时间高亮样式
function getInTimeHighlight(dongle: Dongle): string {
  const baseClasses = 'border-2'

  // 入库和损坏状态高亮入库时间
  if (dongle.dongleStatus === '入库' || dongle.dongleStatus === '损坏') {
    switch (dongle.dongleStatus) {
      case '入库':
        return `${baseClasses} bg-orange-100 border-orange-400 text-orange-700`
      case '损坏':
        return `${baseClasses} bg-red-100 border-red-400 text-red-700`
      default:
        return `${baseClasses} bg-gray-50 border-gray-200 text-gray-700`
    }
  }

  // 其他状态使用默认样式
  return `${baseClasses} bg-gray-50 border-gray-200 text-gray-700`
}

// 获取出库时间高亮样式
function getOutTimeHighlight(dongle: Dongle): string {
  const baseClasses = 'border-2'

  // 出库和丢失状态高亮出库时间
  if (dongle.dongleStatus === '出库' || dongle.dongleStatus === '丢失') {
    switch (dongle.dongleStatus) {
      case '出库':
        return `${baseClasses} bg-green-100 border-green-400 text-green-700`
      case '丢失':
        return `${baseClasses} bg-purple-100 border-purple-400 text-purple-700`
      default:
        return `${baseClasses} bg-gray-50 border-gray-200 text-gray-700`
    }
  }

  // 其他状态使用默认样式
  return `${baseClasses} bg-gray-50 border-gray-200 text-gray-700`
}

// 获取状态对应的时间（已存在，确保正确）
function getStatusTime(dongle: Dongle): string {
  switch (dongle.dongleStatus) {
    case '出库':
    case '丢失':
      return dongle.outTime || '未设置'
    case '入库':
    case '损坏':
      return dongle.inTime || '未设置'
    default:
      return '未设置'
  }
}

// 获取加密锁状态对应的颜色
function getDongleStatusColor(dongle: Dongle): string {
  switch (dongle.dongleStatus) {
    case '出库':
      return 'green' // 出库 - 绿色
    case '入库':
      return 'yellow' // 入库 - 黄色
    case '损坏':
    case '丢失':
      return 'gray' // 损坏/丢失 - 灰色
    default:
      return 'gray' // 默认 - 灰色
  }
}

// 获取加密锁状态对应的提示文字
function getDongleStatusTooltip(dongle: Dongle): string {
  switch (dongle.dongleStatus) {
    case '出库':
      return '已出库'
    case '入库':
      return '已入库'
    case '损坏':
      return '设备损坏'
    case '丢失':
      return '设备丢失'
    default:
      return '状态未知'
  }
}

// 获取状态对应的时间标签（已存在，确保正确）
function getStatusTimeLabel(dongle: Dongle): string {
  switch (dongle.dongleStatus) {
    case '出库':
    case '丢失':
      return '出库时间'
    case '入库':
    case '损坏':
      return '入库时间'
    default:
      return '时间'
  }
}
// -------------后端事件处理函数---------------
// 获取加密锁列表数据
async function fetchDongles() {
  loading.value = true
  try {
    // 调用真实的API接口
    const res = await deliveryApi.getDongleList()

    if (res?.data) {
      // 数据映射处理，确保与前端接口一致
      const mappedData: Dongle[] = (res.data.list || []).map((dongle: any) => ({
        dongleId: dongle.dongleId || '', // 加密锁ID
        shellCode: dongle.shellCode || '', // 外壳号
        shellSerial: dongle.shellSerial || '', // 外壳序列号
        dongleRemark: dongle.dongleRemark || '', // 备注
        dongleStatus: dongle.dongleStatus || '', // 加密锁状态
        clientName: dongle.clientName || '', // 客户名称
        clientDeviceType: dongle.clientDeviceType || '', // 客户设备类型
        clientNote: dongle.clientNote || '', // 客户电脑备注
        inTime: dongle.inTime || '', // 入库时间
        outTime: dongle.outTime || '', // 出库时间
      }))

      dongleList.value = mappedData
      console.warn('获取加密锁列表成功:', mappedData)
    }
    else {
      throw new Error('响应数据为空')
    }
  }
  catch (error) {
    console.error('获取加密锁列表失败:', error)
    ElMessage.error('获取加密锁列表失败，请稍后重试')
  }
  finally {
    loading.value = false
  }
}

// -------------------事件处理函数-------------------

// ------------------编辑弹窗相关------------------------
const editDialogVisible = ref(false)
const editingDongle = ref<Dongle | null>(null)
const editFormRef = ref()
const editForm = ref({
  shellCode: '', // 外壳号
  shellSerial: '', // 序列号
})

// 编辑表单验证规则
const editRules = {
  shellCode: [
    { required: true, message: '请输入外壳号', trigger: 'blur' },
    { validator: validateShellCode, trigger: 'blur' },
  ],
  shellSerial: [
    { required: true, message: '请输入序列号', trigger: 'blur' },
    { validator: validateShellSerial, trigger: 'blur' },
  ],
}

// 更新外壳号后4位的方法
function updateShellCodeSuffix(value: string) {
  // 确保只输入数字
  const numericValue = value.replace(/\D/g, '')

  // 限制长度为4位
  const limitedValue = numericValue.slice(0, 4)

  // 补齐4位，不足的前面补0
  const paddedValue = limitedValue.padStart(4, '0')

  // 更新完整的外壳号
  const prefix = editForm.value.shellCode.substring(0, 8) // A5300000
  editForm.value.shellCode = prefix + paddedValue
}

// 打开编辑弹窗
function handleEdit(dongle: Dongle) {
  editingDongle.value = dongle
  editForm.value = {
    shellCode: dongle.shellCode,
    shellSerial: dongle.shellSerial,
  }
  editDialogVisible.value = true
}

// 保存编辑
async function saveEdit() {
  try {
    // 表单验证
    const valid = await editFormRef.value?.validate()
    if (!valid) {
      console.warn('表单验证失败')
      return
    }

    // 调用后端API保存编辑数据
    const res = await deliveryApi.updateDongle({
      dongleId: editingDongle.value?.dongleId ?? '',
      shellCode: editForm.value.shellCode,
      shellSerial: editForm.value.shellSerial,
    })

    if (res?.data?.success) {
      editDialogVisible.value = false
      ElMessage.success(res.data.message || '更新加密锁信息成功')

      // 重新获取列表数据
      await fetchDongles()
    }
    else {
      ElMessage.error(res?.data?.message || '更新加密锁信息失败')
    }
  }
  catch (error) {
    console.error('保存失败:', error)
    ElMessage.error('更新加密锁信息失败')
  }
}

// 取消编辑
function cancelEdit() {
  editDialogVisible.value = false
  editForm.value = {
    shellCode: '',
    shellSerial: '',
  }
  editingDongle.value = null
}

// -------------历史记录弹窗相关-------------------
const historyDialogVisible = ref(false)
const historyList = ref<DongleHistory[]>([])
const currentDongleCode = ref('')

// 历史记录数据结构（保持现有字段不变）
interface DongleHistory {
  id: string
  outTime: string // 出库时间
  inTime: string // 入库时间
  clientName: string // 客户名称
  authorizations: Authorization[] // 该时间段内的授权列表
}

// 新增授权信息结构
interface Authorization {
  generateDate: string // 生成日期（产品授权信息表.生成日期）
  authId: string // 授权ID
  startDate: string // 授权开始日期
  endDate: string // 授权结束日期
  authType: string // 授权类型
  authNote: string // 授权备注
}
// 新增历史记录加载状态
const historyLoading = ref(false)

// 从后端获取特定加密锁的历史记录
async function fetchDongleHistory(dongleId: string) {
  historyLoading.value = true
  try {
    // 调用后端API获取历史记录
    const res = await deliveryApi.getDongleHistory(dongleId)

    if (res?.data) {
      // 数据映射处理 - 将后端字段映射到新的前端结构
      const mappedHistory: DongleHistory[] = (res.data.list || []).map((history: any) => ({
        id: history.id || '',
        outTime: history.outTime || '未知', // 出库时间
        inTime: history.inTime || '未知', // 入库时间
        clientName: history.clientName || '暂无客户',
        authorizations: (history.authorizations || []).map((auth: any) => ({
          generateDate: auth.generateDate || '',
          authId: auth.authId || '',
          startDate: auth.startDate || '',
          endDate: auth.endDate || '',
          authType: auth.authType || '',
          authNote: auth.authNote || '',
        })),
      }))

      historyList.value = mappedHistory
    }
    else {
      historyList.value = []
      console.warn('没有找到历史记录')
    }
  }
  catch (error) {
    console.error('获取历史记录失败:', error)
    ElMessage.error('获取历史记录失败，请稍后重试')
    historyList.value = []
  }
  finally {
    historyLoading.value = false
  }
}

// 查看历史记录（修改为使用dongleId从后端获取）
async function handleHistory(dongle: Dongle) {
  currentDongleCode.value = dongle.shellCode

  // 打开弹窗
  historyDialogVisible.value = true

  // 清空之前的数据
  historyList.value = []

  // 使用dongleId从后端获取历史记录
  await fetchDongleHistory(dongle.shellCode)

  console.warn('查看历史记录:', dongle.shellCode, 'dongleId:', dongle.dongleId)
}

// 关闭历史记录弹窗
function closeHistoryDialog() {
  historyDialogVisible.value = false
  historyList.value = []
  currentDongleCode.value = ''
  historyLoading.value = false
}

// -----------------创建加密锁相关-------------------
const createDialogVisible = ref(false)
const createFormRef = ref()
const createForm = ref({
  shellCode: '', // 外壳号 - 必填
  shellSerial: '', // 序列号 - 必填
})

// 外壳号格式验证：A + 11位数字（前7位固定，后4位可修改）
const shellCodePattern = /^A\d{11}$/
// 序列号格式验证：32位字母数字混合
const shellSerialPattern = /^[A-Z0-9]{32}$/i

// 外壳号验证函数
function validateShellCode(_: any, value: string, callback: any) {
  if (!value) {
    callback(new Error('请输入外壳号'))
    return
  }
  if (!shellCodePattern.test(value)) {
    callback(new Error('外壳号格式错误，应为A开头+11位数字，如：A53000000001'))
    return
  }
  callback()
}

// 序列号验证函数
function validateShellSerial(_: any, value: string, callback: any) {
  if (!value) {
    callback(new Error('请输入序列号'))
    return
  }
  if (!shellSerialPattern.test(value)) {
    callback(new Error('序列号格式错误，应为32位字母数字组合'))
    return
  }
  callback()
}

// 更新创建表单外壳号后4位的方法
function updateCreateShellCodeSuffix(value: string) {
  // 确保只输入数字
  const numericValue = value.replace(/\D/g, '')

  // 限制长度为4位
  const limitedValue = numericValue.slice(0, 4)

  // 更新完整的外壳号（不需要补0，让用户自己输入）
  createForm.value.shellCode = `A5300000${limitedValue}`
}

// 修改创建表单验证规则
const createRules = {
  shellCode: [
    { required: true, message: '请输入外壳号', trigger: 'blur' },
    { validator: validateShellCode, trigger: 'blur' },
  ],
  shellSerial: [
    { required: true, message: '请输入序列号', trigger: 'blur' },
    { validator: validateShellSerial, trigger: 'blur' },
  ],
}
// 打开创建弹窗
function handleCreate() {
  // 重置表单为默认值 - 设置完整的12位初始值
  createForm.value = {
    shellCode: 'A53000000000', // 完整12位：A5300000 + 0000
    shellSerial: '',
  }
  createDialogVisible.value = true
}
// 保存创建
async function saveCreate() {
  try {
    // 表单验证
    const valid = await createFormRef.value?.validate()
    if (!valid) {
      console.warn('表单验证失败')
      return
    }

    // 调用后端API创建加密锁
    const res = await deliveryApi.createDongle({
      shellCode: createForm.value.shellCode,
      shellSerial: createForm.value.shellSerial,
    })

    if (res?.data?.success) {
      createDialogVisible.value = false
      ElMessage.success(res.data.message || '创建加密锁成功')

      // 重新获取列表数据
      await fetchDongles()
    }
    else {
      ElMessage.error(res?.data?.message || '创建加密锁失败')
    }
  }
  catch (error) {
    console.error('创建失败:', error)
    ElMessage.error('创建加密锁失败')
  }
}

// 取消创建
function cancelCreate() {
  createDialogVisible.value = false
  createForm.value = {
    shellCode: 'A53000000000', // 重置为完整的12位
    shellSerial: '',
  }
}

// 授权详情跳转处理 - 跳转到客户管理页面并高亮对应客户
function handleAuthDetail(dongle: Dongle) {
  // 检查是否有客户信息
  if (!dongle.clientName || dongle.clientName.trim() === '') {
    ElMessage.warning('该加密锁尚未分配给客户，无法查看授权详情')
    return
  }

  // 跳转到客户管理页面，传递客户信息和加密锁信息用于高亮
  router.push({
    path: '/client_manage', // 跳转到客户管理页面
    query: {
      highlightClient: dongle.clientName, // 需要高亮的客户名称
      fromDongleManage: 'true', // 标识来源于加密锁管理页面
      dongleCode: dongle.shellCode, // 加密锁外壳号，用于额外信息展示
      dongleId: dongle.dongleId, // 加密锁ID，用于后续操作
    },
  })
}

// 页面初始化时加载第一页数据
onMounted(() => {
  fetchDongles()
})

// 控制每个加密锁的展开/收起
function expandDongle(shellCode: string, expand: boolean) {
  expandedMap.value[shellCode] = expand
}
</script>

<template>
  <div>
    <el-skeleton :loading="loading" animated>
      <template #default>
        <!-- 顶部标题栏 -->
        <div class="mb-6 border border-gray-200 rounded-lg bg-white p-6 shadow-sm">
          <div class="flex items-center justify-between">
            <div class="flex items-center">
              <h1 class="text-2xl text-gray-800 font-bold">
                加密锁管理
              </h1>
            </div>
            <div class="flex items-center">
              <FaButton
                class="border border-green-500 rounded bg-green-300 px-5 py-2 text-black font-semibold transition-colors duration-150 hover:bg-green-400"
                @click="handleCreate"
              >
                创建加密锁
              </FaButton>
            </div>
          </div>

          <!-- 文字提示行 -->
          <div class="mt-3 text-sm text-gray-600">
            本页面为加密锁管理页面，提供加密锁的创建、编辑和历史记录查看功能
          </div>
        </div>
        <!-- 动态渲染所有加密锁 -->
        <FaPageMain
          v-for="dongle in dongleList"
          :key="dongle.shellCode"
          :title="`外壳号 ${dongle.shellCode}`"
          :collaspe="!expandedMap[dongle.shellCode]"
          height="auto"
          class="w-full"
        >
          <template #title>
            <div class="w-full">
              <!-- 标题行 -->
              <div class="flex items-center gap-2">
                <!-- 状态图标 -->
                <el-tooltip
                  :content="getDongleStatusTooltip(dongle)"
                  placement="top"
                >
                  <div
                    class="rounded-full"
                    :class="{
                      'w-4 h-4 bg-green-500': getDongleStatusColor(dongle) === 'green',
                      'w-4 h-4 bg-yellow-500': getDongleStatusColor(dongle) === 'yellow',
                      'w-4 h-4 bg-gray-400': getDongleStatusColor(dongle) === 'gray',
                    }"
                  />
                </el-tooltip>
                <span class="ml-2 text-2xl font-bold">
                  {{ dongle.shellCode }}
                </span>

                <!-- 右上角按钮区域 -->
                <div class="ml-auto mr-4 flex gap-2">
                  <FaButton
                    class="border border-blue-500 rounded bg-blue-300 px-5 py-1.5 text-black font-semibold transition-colors duration-150 hover:bg-blue-400"
                    @click="handleEdit(dongle)"
                  >
                    编辑
                  </FaButton>
                  <FaButton
                    class="border border-purple-500 rounded bg-purple-300 px-5 py-1.5 text-black font-semibold transition-colors duration-150 hover:bg-purple-400"
                    @click="handleHistory(dongle)"
                  >
                    历史记录
                  </FaButton>
                  <FaButton
                    class="border border-orange-500 rounded bg-orange-300 px-5 py-1.5 text-black font-semibold transition-colors duration-150 hover:bg-orange-400"
                    @click="handleAuthDetail(dongle)"
                  >
                    授权详情
                  </FaButton>
                </div>
              </div>

              <!-- 展开/收起按钮和状态信息行 -->
              <div class="ml-2 mt-2 flex items-center gap-4">
                <FaIcon
                  name="expand"
                  class="mr-2 cursor-pointer text-xl"
                  @click.stop="expandDongle(dongle.shellCode, !expandedMap[dongle.shellCode])"
                />
                <span class="text-base text-black">
                  <span class="font-bold">状态：</span>{{ dongle.dongleStatus }}
                </span>
                <span class="text-base text-black">
                  <span class="font-bold">{{ getStatusTimeLabel(dongle) }}：</span>{{ getStatusTime(dongle) }}
                </span>
                <span class="text-base text-black">
                  <span class="font-bold">用户：</span>{{ dongle.clientName }}
                </span>
              </div>
            </div>
          </template>

          <!-- 展开内容 -->
          <div class="space-y-6">
            <!-- 加密锁信息 -->
            <div class="border border-blue-200 rounded-lg p-6">
              <h3 class="mb-4 text-lg text-black font-bold">
                密钥信息
              </h3>
              <div class="grid grid-cols-2 items-start gap-x-8 gap-y-4">
                <!-- 外壳号 -->
                <div class="flex items-center gap-2">
                  <span class="w-32 text-black font-semibold">外壳号：</span>
                  <input
                    class="flex-1 border border-gray-200 rounded bg-gray-50 px-3 py-2 text-sm text-blue-700 font-bold"
                    :value="dongle.shellCode"
                    readonly
                  >
                </div>

                <!-- 序列号 -->
                <div class="flex items-center gap-2">
                  <span class="w-32 text-black font-semibold">序列号：</span>
                  <input
                    class="flex-1 border border-gray-200 rounded bg-gray-50 px-3 py-2 text-sm text-blue-700 font-bold"
                    :value="dongle.shellSerial"
                    readonly
                  >
                </div>

                <!-- 状态 -->
                <div class="flex items-center gap-2">
                  <span class="w-32 text-black font-semibold">状态：</span>
                  <input
                    class="flex-1 border border-gray-200 rounded bg-gray-50 px-3 py-2 text-sm text-blue-700 font-bold"
                    :value="dongle.dongleStatus"
                    readonly
                  >
                </div>

                <!-- 客户设备类型 -->
                <div class="flex items-center gap-2">
                  <span class="w-32 text-black font-semibold">客户设备类型：</span>
                  <input
                    class="flex-1 border border-gray-200 rounded bg-gray-50 px-3 py-2 text-sm text-blue-700 font-bold"
                    :value="dongle.clientDeviceType || '未设置'"
                    readonly
                  >
                </div>

                <!-- 入库时间（根据状态决定是否高亮） -->
                <div class="flex items-center gap-2">
                  <span class="w-32 text-black font-semibold">入库时间：</span>
                  <input
                    class="flex-1 border border-gray-200 rounded px-3 py-2 text-sm font-bold"
                    :class="getInTimeHighlight(dongle)"
                    :value="dongle.inTime || '未设置'"
                    readonly
                  >
                </div>

                <!-- 出库时间（根据状态决定是否高亮） -->
                <div class="flex items-center gap-2">
                  <span class="w-32 text-black font-semibold">出库时间：</span>
                  <input
                    class="flex-1 border border-gray-200 rounded px-3 py-2 text-sm font-bold"
                    :class="getOutTimeHighlight(dongle)"
                    :value="dongle.outTime || '未出库'"
                    readonly
                  >
                </div>

                <!-- 客户电脑备注（占满两列） -->
                <div class="col-span-2 flex items-start gap-2">
                  <span class="w-32 text-black font-semibold">客户电脑备注：</span>
                  <textarea
                    class="flex-1 resize-none border border-gray-200 rounded bg-gray-50 px-3 py-2 text-sm text-blue-700 font-bold"
                    :value="dongle.clientNote || '无备注'"
                    rows="2"
                    readonly
                  />
                </div>

                <!-- 备注（占满两列） -->
                <div class="col-span-2 flex items-start gap-2">
                  <span class="w-32 text-black font-semibold">备注：</span>
                  <textarea
                    class="flex-1 resize-none border border-gray-200 rounded bg-gray-50 px-3 py-2 text-sm text-blue-700 font-bold"
                    :value="dongle.dongleRemark || '无备注'"
                    rows="2"
                    readonly
                  />
                </div>
              </div>
            </div>
            <div class="mt-6" />
          </div>
        </FaPageMain>

        <!-- 编辑弹窗 -->
        <el-dialog
          v-model="editDialogVisible"
          title="编辑加密锁信息"
          width="50vw"
          :close-on-click-modal="false"
        >
          <el-form
            ref="editFormRef"
            :model="editForm"
            :rules="editRules"
            label-width="80px"
          >
            <el-form-item label="外壳号" prop="shellCode">
              <div class="flex items-center gap-2">
                <!-- 固定前缀部分 -->
                <el-input
                  :model-value="editForm.shellCode.substring(0, 8)"
                  style="width: 120px;"
                  disabled
                  class="shell-code-prefix"
                />
                <!-- 可编辑的后4位 -->
                <el-input
                  :model-value="editForm.shellCode.substring(8)"
                  style="width: 80px;"
                  maxlength="4"
                  placeholder="0001"
                  @input="updateShellCodeSuffix"
                />
              </div>
              <div class="mt-1 text-xs text-gray-500">
                格式：A + 11位数字，前7位固定(A5300000)，后4位可修改
              </div>
            </el-form-item>

            <el-form-item label="序列号" prop="shellSerial">
              <el-input
                v-model="editForm.shellSerial"
                placeholder="32位字母数字组合"
                maxlength="32"
                show-word-limit
              />
              <div class="mt-1 text-xs text-gray-500">
                格式：32位字母数字组合（区分大小写）
              </div>
            </el-form-item>
          </el-form>

          <template #footer>
            <div class="dialog-footer">
              <el-button @click="cancelEdit">
                取消
              </el-button>
              <el-button type="primary" @click="saveEdit">
                保存
              </el-button>
            </div>
          </template>
        </el-dialog>

        <!-- 历史记录弹窗 -->
        <el-dialog
          v-model="historyDialogVisible"
          :title="`${currentDongleCode} - 授权历史记录`"
          width="80vw"
          :close-on-click-modal="false"
        >
          <div class="history-content">
            <!-- 历史记录加载状态 -->
            <div v-if="historyLoading" class="py-8 text-center">
              <el-icon class="mr-2 animate-spin">
                <Loading />
              </el-icon>
              <span class="text-gray-600">正在加载历史记录...</span>
            </div>

            <!-- 历史记录列表 - 新的展示方式 -->
            <div v-else class="space-y-6">
              <!-- 每个历史记录段 -->
              <div
                v-for="history in historyList"
                :key="history.id"
                class="border border-gray-200 rounded-lg bg-white p-4 shadow-sm"
              >
                <!-- 头部信息：出库时间、入库时间和客户 -->
                <div class="mb-4 border-b border-gray-100 pb-3">
                  <div class="flex items-center justify-between">
                    <div class="flex items-center gap-6">
                      <!-- 出库时间 -->
                      <div class="flex items-center">
                        <el-icon class="mr-2 text-green-500">
                          <ArrowUp />
                        </el-icon>
                        <span class="text-sm text-gray-600 font-medium">出库时间：</span>
                        <span class="ml-1 text-base text-green-700 font-semibold">{{ history.outTime }}</span>
                      </div>

                      <!-- 入库时间 -->
                      <div class="flex items-center">
                        <el-icon class="mr-2 text-orange-500">
                          <ArrowDown />
                        </el-icon>
                        <span class="text-sm text-gray-600 font-medium">入库时间：</span>
                        <span class="ml-1 text-base text-orange-700 font-semibold">{{ history.inTime }}</span>
                      </div>

                      <!-- 客户信息 -->
                      <div class="flex items-center">
                        <el-icon class="mr-2 text-blue-500">
                          <User />
                        </el-icon>
                        <span class="text-sm text-gray-600 font-medium">客户：</span>
                        <span class="ml-1 text-base text-blue-700 font-semibold">{{ history.clientName || '暂无客户' }}</span>
                      </div>
                    </div>

                    <div class="text-sm text-gray-500">
                      共 {{ history.authorizations.length }} 个授权
                    </div>
                  </div>
                </div>

                <!-- 授权信息列表 -->
                <div v-if="history.authorizations.length > 0" class="space-y-3">
                  <h4 class="mb-3 text-sm text-gray-600 font-semibold">
                    该时间段内的授权信息：
                  </h4>

                  <!-- 授权信息表格 -->
                  <el-table
                    :data="history.authorizations"
                    size="small"
                    stripe
                    border
                    class="w-full"
                  >
                    <el-table-column
                      prop="generateDate"
                      label="生成日期"
                      width="120"
                      sortable
                    >
                      <template #default="{ row }">
                        <div class="text-center">
                          <el-tag type="info" size="small">
                            {{ row.generateDate }}
                          </el-tag>
                        </div>
                      </template>
                    </el-table-column>

                    <el-table-column
                      prop="authId"
                      label="授权ID"
                      width="140"
                    >
                      <template #default="{ row }">
                        <div class="text-blue-600 font-semibold">
                          {{ row.authId }}
                        </div>
                      </template>
                    </el-table-column>

                    <el-table-column
                      prop="startDate"
                      label="授权开始日期"
                      width="120"
                    >
                      <template #default="{ row }">
                        <div class="text-green-600">
                          {{ row.startDate }}
                        </div>
                      </template>
                    </el-table-column>

                    <el-table-column
                      prop="endDate"
                      label="授权结束日期"
                      width="120"
                    >
                      <template #default="{ row }">
                        <div class="text-orange-600">
                          {{ row.endDate }}
                        </div>
                      </template>
                    </el-table-column>

                    <el-table-column
                      prop="authType"
                      label="授权类型"
                      width="100"
                    >
                      <template #default="{ row }">
                        <el-tag
                          :type="row.authType === '本地锁' ? 'primary' : row.authType === '网络锁' ? 'success' : 'warning'"
                          size="small"
                        >
                          {{ row.authType }}
                        </el-tag>
                      </template>
                    </el-table-column>

                    <el-table-column
                      prop="authNote"
                      label="授权备注"
                      min-width="200"
                      show-overflow-tooltip
                    >
                      <template #default="{ row }">
                        <div class="text-sm text-gray-600">
                          {{ row.authNote || '无备注' }}
                        </div>
                      </template>
                    </el-table-column>
                  </el-table>
                </div>

                <!-- 无授权信息提示 -->
                <div v-else class="py-6 text-center text-gray-400">
                  <el-icon class="mb-2 text-2xl">
                    <Warning />
                  </el-icon>
                  <div class="text-sm">
                    该时间段内无授权信息
                  </div>
                </div>
              </div>
            </div>

            <!-- 无数据提示 -->
            <div v-if="!historyLoading && historyList.length === 0" class="py-8 text-center text-gray-400">
              <el-icon class="mb-2 text-4xl">
                <Document />
              </el-icon>
              <div>暂无历史记录</div>
            </div>
          </div>

          <template #footer>
            <div class="dialog-footer">
              <el-button @click="closeHistoryDialog">
                关闭
              </el-button>
            </div>
          </template>
        </el-dialog>

        <!-- 创建加密锁弹窗 -->
        <el-dialog
          v-model="createDialogVisible"
          title="创建新的加密锁"
          width="60vw"
          :close-on-click-modal="false"
        >
          <el-form
            ref="createFormRef"
            :model="createForm"
            :rules="createRules"
            label-width="80px"
          >
            <el-form-item label="外壳号" prop="shellCode">
              <div class="flex items-center gap-2">
                <!-- 固定前缀 -->
                <el-input
                  :model-value="createForm.shellCode.substring(0, 8)"
                  style="width: 120px;"
                  disabled
                  class="shell-code-prefix"
                />
                <!-- 可输入的后4位 -->
                <el-input
                  :model-value="createForm.shellCode.substring(8)"
                  style="width: 80px;"
                  maxlength="4"
                  placeholder="0001"
                  @input="updateCreateShellCodeSuffix"
                />
              </div>
              <div class="mt-1 text-xs text-gray-500">
                格式：A5300000 + 4位数字，如：A53000000001
              </div>
            </el-form-item>

            <el-form-item label="序列号" prop="shellSerial">
              <el-input
                v-model="createForm.shellSerial"
                placeholder="32位字母数字组合"
                maxlength="32"
                show-word-limit
              />
              <div class="mt-1 text-xs text-gray-500">
                格式：32位字母数字组合（区分大小写）
              </div>
            </el-form-item>
          </el-form>

          <template #footer>
            <div class="dialog-footer">
              <el-button @click="cancelCreate">
                取消
              </el-button>
              <el-button type="primary" @click="saveCreate">
                创建
              </el-button>
            </div>
          </template>
        </el-dialog>
      </template>
    </el-skeleton>
  </div>
</template>
