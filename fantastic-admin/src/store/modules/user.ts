import { ElMessage } from 'element-plus'
import apiUser from '@/api/modules/user'
import router from '@/router'
import { useClientSuffixStore } from './clientSuffix'

export const useUserStore = defineStore(
  // 唯一ID
  'user',
  () => {
    // 获取其它全局状态模块（设置、路由、菜单、标签栏）
    const settingsStore = useSettingsStore()
    const routeStore = useRouteStore()
    const menuStore = useMenuStore()
    const tabbarStore = useTabbarStore()
    const clientSuffixStore = useClientSuffixStore()

    // 用户账号（从本地存储读取，支持刷新后自动恢复）
    const account = ref(localStorage.account ?? '')
    // 用户待处理工单数量
    const pendingWorkOrdersCount = ref(0)
    // 修改：使用sessionStorage而不是localStorage存储token
    const token = ref(sessionStorage.token ?? '')
    // 用户头像（从本地存储读取）
    const avatar = ref(localStorage.avatar ?? '')
    // 用户权限列表（如菜单、操作权限等）
    const permissions = ref<string[]>([])
    // 用户角色（从本地存储读取，支持刷新后自动恢复）
    const role = ref(localStorage.role ?? '')
    // 用户是否有审批身份（从本地存储读取，支持刷新后自动恢复）
    const userModels = ref<string[]>(
      (() => {
        const raw = localStorage.getItem('models')
        if (!raw || raw === 'undefined') {
          return []
        }
        try {
          return JSON.parse(raw)
        }
        catch {
          return []
        }
      })(),
    )

    // 是否已登录（根据 token 是否存在判断）
    const isLogin = computed(() => {
      if (token.value) {
        // 可选：添加token过期检查
        return !isTokenExpired(token.value)
      }
      return false
    })

    // 新增：检查token是否过期的简单函数
    function isTokenExpired(tokenValue: string): boolean {
      try {
        // 简单的JWT token过期检查（如果使用JWT）
        const payload = JSON.parse(atob(tokenValue.split('.')[1]))
        const currentTime = Math.floor(Date.now() / 1000)
        return payload.exp < currentTime
      }
      catch {
        // 如果解析失败，认为token无效
        return true
      }
    }

    // 登录
    async function login(data: {
      account: string
      password: string
    }) {
      const res = await apiUser.login(data)

      if (res.data === '') {
        ElMessage.error('登录失败，请检查用户名和密码')
        return
      }

      // 确保models数据格式正确
      const modelsData = Array.isArray(res.data.models)
        ? res.data.models
        : (res.data.models ? [res.data.models] : [])

      // 存储用户基本信息
      localStorage.setItem('account', res.data.account) // 登录用户名
      sessionStorage.setItem('token', res.data.token) // jwt令牌
      localStorage.setItem('role', res.data.role) // 用户角色
      localStorage.setItem('isApprover', res.data.isApprover ? 'true' : 'false') // 审批身份
      localStorage.setItem('models', JSON.stringify(modelsData))

      account.value = res.data.account
      token.value = res.data.token
      avatar.value = res.data.avatar
      userModels.value = modelsData

      // 登录成功后获取客户后缀数据
      try {
        await clientSuffixStore.fetchClientSuffixes()
      }
      catch (error) {
        console.warn('获取客户后缀数据失败，但不影响登录:', error)
      }
    }

    // 手动登出
    function logout(redirect = router.currentRoute.value.fullPath) {
      sessionStorage.removeItem('token') // 改为sessionStorage
      token.value = ''
      router.push({
        name: 'login',
        query: {
          ...(redirect !== settingsStore.settings.home.fullPath && router.currentRoute.value.name !== 'login' && { redirect }),
        },
      }).then(logoutCleanStatus)
    }
    // 请求登出
    function requestLogout() {
      sessionStorage.removeItem('token') // 改为sessionStorage
      token.value = ''
      router.push({
        name: 'login',
        query: {
          ...(
            router.currentRoute.value.fullPath !== settingsStore.settings.home.fullPath
            && router.currentRoute.value.name !== 'login'
            && {
              redirect: router.currentRoute.value.fullPath,
            }
          ),
        },
      }).then(logoutCleanStatus)
    }
    // 登出后清除状态
    function logoutCleanStatus() {
      localStorage.removeItem('account')
      localStorage.removeItem('avatar')
      localStorage.removeItem('role') // 清除用户角色
      localStorage.removeItem('isApprover') // 清除审批身份
      localStorage.removeItem('models') // 清除负责模型
      account.value = ''
      avatar.value = ''
      userModels.value = []
      permissions.value = []
      settingsStore.updateSettings({}, true)
      tabbarStore.clean()
      routeStore.removeRoutes()
      menuStore.setActived(0)
      // 清除客户后缀数据
      clientSuffixStore.clearSuffixData()
    }

    // 获取权限
    async function getPermissions() {
      const res = await apiUser.permission({ userID: account.value })
      permissions.value = res.data.permissions
      console.warn('获取权限', permissions.value)
    }

    // 修改密码
    async function editPassword(data: {
      account: string
      oldPassword: string
      newPassword: string
    }) {
      await apiUser.changePassword(data)
    }

    return {
      account,
      token,
      avatar,
      permissions,
      role,
      isLogin,
      login,
      logout,
      requestLogout,
      getPermissions,
      userModels,
      pendingWorkOrdersCount,
      editPassword, // 已移除未使用的函数
    }
  },
)
