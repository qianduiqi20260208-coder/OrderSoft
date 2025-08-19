import { faker } from '@faker-js/faker'
import { defineFakeRoute } from 'vite-plugin-fake-server/client'

export default defineFakeRoute([
  {
    url: '/mock/user/login',
    method: 'post',
    response: ({ body }) => {
      // 根据账号模拟不同角色和身份
      let role = ''
      let isApprover = false
      let models: string[] = []
      if (body.account === 'admin') {
        role = 'SuperUser'
        isApprover = true
        models = [
          'ATA04_Aerodynamics',
          'ATA08_WeightBalance',
          'ATA00_Groundhandling',
          'ATA21_AirConditioning',
          'ATA21_PressutizationAndDoors',
          'ATA24_Electric',
          'ATA26_Fire',
          'ATA27_ACEact',
          'ATA27F_FlapsSlats',
          'ATA27W_StallWarningComputer',
          'ATA28_Fuel',
          'ATA29_Hydraulic',
          'ATA30_IceAndRain',
          'ATA32_LandingGear',
          'ATA32_Steering',
          'ATA32_Brakes',
          'ATA33_Lights',
          'ATA34_IRS',
          'ATA34_FlightInstruments',
          'ATA35_Oxygen',
          'ATA36_Penumatic',
          'ATA49_APU',
          'ATA21/30/36/52_ECS',
          'ATA7x_Engine',
        ]
      }
      else if (body.account === 'test') {
        role = '模型工程师'
        isApprover = false
        models = ['ModelA']
      }
      else {
        role = '访客'
        isApprover = false
        models = ['']
      }
      return {
        error: '',
        status: 1,
        data: {
          account: body.account, // 登录用户名
          token: `${body.account}:${faker.internet.jwt()}`, // 模拟jwt令牌
          avatar: 'https://fantastic-admin.hurui.me/logo.svg', // 模拟用户头像
          role, // 用户角色
          isApprover, // 审批身份
          models, // 负责模型
        },
      }
    },
  },
  {
    url: '/mock/user/permission',
    method: 'get',
    response: ({ headers }) => {
      let permissions: string[] = []
      if (headers.token?.indexOf('admin') === 0) {
        permissions = [
          'permission.browse',
          'permission.create',
          'permission.edit',
          'permission.remove',
        ]
      }
      else if (headers.token?.indexOf('test') === 0) {
        permissions = [
          'permission.browse',
        ]
      }
      return {
        error: '',
        status: 1,
        data: {
          permissions,
        },
      }
    },
  },
  // 新增：修改密码mock接口
  {
    url: '/mock/user/changePassword',
    method: 'post',
    response: ({ body }) => {
      const { account } = body

      // 模拟修改成功
      return {
        status: 0,
        message: '密码修改成功',
        data: {
          account,
          success: true,
        },
      }
    },
  },
])
