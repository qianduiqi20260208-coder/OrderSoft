import apiUser from '@/api/modules/user'
import router from '@/router'

export const useUserStore = defineStore(
  // 唯一ID
  'user',
  () => {
    // 获取其它全局状态模块（设置、路由、菜单、标签栏）
    const settingsStore = useSettingsStore()
    const routeStore = useRouteStore()
    const menuStore = useMenuStore()
    const tabbarStore = useTabbarStore()

    // 用户账号（从本地存储读取，支持刷新后自动恢复）
    const account = ref(localStorage.account ?? '')
    // 用户登录令牌（token），用于鉴权
    const token = ref(localStorage.token ?? '')
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
        return true
      }
      return false
    })

    // 登录
    async function login(data: {
      account: string
      password: string
    }) {
      const res = await apiUser.login(data)

      // 存储用户基本信息
      localStorage.setItem('account', res.data.account) // 登录用户名
      localStorage.setItem('token', res.data.token) // jwt令牌
      localStorage.setItem('role', res.data.role) // 用户角色
      localStorage.setItem('isApprover', res.data.isApprover ? 'true' : 'false') // 审批身份

      account.value = res.data.account
      token.value = res.data.token
      avatar.value = res.data.avatar
    }

    // 手动登出
    function logout(redirect = router.currentRoute.value.fullPath) {
      // 此处仅清除计算属性 isLogin 中判断登录状态过期的变量，以保证在弹出登录窗口模式下页面展示依旧正常
      localStorage.removeItem('token')
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
      // 此处仅清除计算属性 isLogin 中判断登录状态过期的变量，以保证在弹出登录窗口模式下页面展示依旧正常
      localStorage.removeItem('token')
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
    }

    // 获取权限
    async function getPermissions() {
      const res = await apiUser.permission()
      permissions.value = res.data.permissions
    }

    // 修改密码
    async function editPassword(data: {
      password: string
      newPassword: string
    }) {
      await apiUser.passwordEdit(data)
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
      editPassword,
      userModels,
    }
  },
)
