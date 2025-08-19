import api from '../index'

export default {
  // 登录
  login: (data: {
    account: string
    password: string
  }) => api.post('user/login', data, {
    // baseURL: '/mock/',
  }),

  // 获取权限
  permission: () => api.get('user/permission', {
    // baseURL: '/mock/',
  }),

  // 修改密码接口
  changePassword: (data: {
    account: string
    oldPassword: string
    newPassword: string
  }) =>
    api.post('user/changePassword', data, {
      baseURL: '/mock/',
    }),
}
