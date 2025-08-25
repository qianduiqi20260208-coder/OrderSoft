// import { faker } from '@faker-js/faker'
import { defineFakeRoute } from 'vite-plugin-fake-server/client'

function generateATA04Models(count = 30) {
  const models = []
  for (let i = 1; i <= count; i++) {
    const version = `V1.0.${i.toString().padStart(2, '0')}.0`
    const updatetime = `2025-07-${(i % 28 + 1).toString().padStart(2, '0')}`
    const orders = []

    // 随机生成工单类型和内容
    if (i % 5 === 0) {
      // 版本迭代+交付发送
      orders.push({
        orderID: `ID${1000 + i}`,
        type: '版本迭代+交付发送',
        startTime: `2025/06/${(i % 28 + 1).toString().padStart(2, '0')}`,
        finishTime: `2025/07/${(i % 28 + 1).toString().padStart(2, '0')}`,
        coordinationID: `C${1000 + i}`,
        apiChanged: i % 2 === 0,
        updateNotes: `第${i}次适配新平台，修复BUG，增加新功能`,
        packingRequirement: i % 2 === 0 ? '高安全封装' : '标准封装',
        promoterID: `user${i}`,
        finisherID: `user${i}`,
        isCAEChecked: i % 2 === 0,
        isEncrypted: i % 3 === 0,
        targetCustomer: `客户${i}`,
        finishAuthId: `AUTH-2025${i}`,
        finishShellNo: `SHELL-${1000 + i}`,
      })
    }
    else {
      // 版本迭代
      orders.push({
        orderID: `I${1000 + i}`,
        type: '版本迭代',
        startTime: `2025/06/${(i % 28 + 1).toString().padStart(2, '0')}`,
        finishTime: `2025/07/${(i % 28 + 1).toString().padStart(2, '0')}`,
        coordinationID: i % 2 === 0 ? `C${2000 + i}` : undefined,
        apiChanged: i % 2 === 1,
        updateNotes: `第${i}次优化性能`,
        packingRequirement: i % 2 === 1 ? '标准封装' : '紧急封装',
        promoterID: `user${i}`,
        finisherID: `user${i}`,
      })
      // 交付发送
      orders.push({
        orderID: `D${1000 + i}`,
        type: '交付发送',
        targetCustomer: `客户${i}`,
        finishAuthId: `AUTH-2025${i}`,
        finishShellNo: `SHELL-${2000 + i}`,
        promoterID: `user${i}`,
        finisherID: `user${i}`,
        finishTime: `2025/07/${(i % 28 + 1).toString().padStart(2, '0')}`,
        isEncrypted: i % 3 === 0,
        isCAEChecked: i % 2 === 0,
      })
      // 功能开发
      if (i % 3 === 0) {
        orders.push({
          orderID: `F${1000 + i}`,
          type: '功能开发',
          finishFeatureDesc: `第${i}次发动机自动启停与节能控制，支持多种工作模式。`,
          promoterID: `user${i}`,
          finisherID: `user${i}`,
          finishTime: `2025/07/${(i % 28 + 1).toString().padStart(2, '0')}`,
        })
      }
      // 问题复现
      if (i % 4 === 0) {
        orders.push({
          orderID: `R${1000 + i}`,
          type: '问题复现',
          finishPhenomenon: `第${i}次发动机启动异常，偶发卡滞`,
          finishRemark: `第${i}次需进一步分析数据`,
          coordinationID: i % 2 === 0 ? `C${3000 + i}` : undefined,
          promoterID: `user${i}`,
          finisherID: `user${i}`,
          finishTime: `2025/07/${(i % 28 + 1).toString().padStart(2, '0')}`,
        })
      }
    }

    models.push({
      modelID: 'ATA04_Aerodynamics',
      version,
      updatetime,
      orders,
    })
  }
  return models
}

export default defineFakeRoute([
  {
    url: '/mock/model/list',
    method: 'get',
    response: ({ query }) => {
      let data: any[] = [
        {
          modelID: 'ATA04_Aerodynamics',
          version: 'V1.0.1.1',
          updatetime: '2025-01-01',
          orders: [
            {
              orderID: 'F001',
              type: '功能开发',
              finishFeatureDesc: '发动机自动启停与节能控制，支持多种工作模式。',
              promoterID: 'alice',
              finisherID: 'alice',
              finishTime: '2024/11/20',
            },
            {
              orderID: 'R001',
              type: '问题复现',
              finishPhenomenon: '发动机启动异常，偶发卡滞',
              finishRemark: '需进一步分析数据',
              coordinationSheet: true,
              coordinationID: 'C001',
              promoterID: 'bob',
              finisherID: 'bob',
              finishTime: '2024/11/22',
            },
            {
              orderID: 'D001',
              type: '交付发送',
              targetCustomer: '华模',
              finishAuthId: 'AUTH-123456',
              finishShellNo: 'SHELL-001',
              promoterID: 'carol',
              finisherID: 'carol',
              finishTime: '2024/11/25',
              isEncrypted: true,
              isCAEChecked: true,
            },
            {
              orderID: 'I001',
              type: '版本迭代',
              startTime: '2024/11/10',
              finishTime: '2024/11/30',
              coordinationSheet: true,
              coordinationID: 'C002',
              apiChanged: false,
              updateNotes: '适配新平台，优化性能',
              packingRequirement: '标准封装',
              promoterID: 'dave',
              finisherID: 'dave',
            },
          ],
        },
        {
          modelID: 'ATA04_Aerodynamics',
          version: 'V1.0.2.0',
          updatetime: '2025-07-01',
          orders: [
            {
              orderID: 'ID1001',
              type: '版本迭代+交付发送',
              startTime: '2025/06/10',
              finishTime: '2025/06/30',
              coordinationID: 'C1001',
              apiChanged: true,
              updateNotes: '适配新平台，修复若干BUG，增加新功能',
              packingRequirement: '高安全封装',
              promoterID: 'lucas',
              finisherID: 'lucas',
              // 交付发送相关
              isCAEChecked: true,
              isEncrypted: false,
              targetCustomer: '新航科技',
              finishAuthId: 'AUTH-20250701',
              finishShellNo: 'SHELL-1001',
            },
          ],
        },
        {
          modelID: 'ATA00_Groundhandling',
          version: 'V2.0.0.0',
          updatetime: '2025-02-15',
          orders: [
            {
              orderID: 'F002',
              type: '功能开发',
              finishFeatureDesc: '地面操作自动检测与提示功能。',
              promoterID: 'eva',
              finisherID: 'eva',
              finishTime: '2025/02/16',
            },
            {
              orderID: 'R002',
              type: '问题复现',
              finishPhenomenon: '地面操作异常报警误报',
              finishRemark: '需优化传感器算法',
              promoterID: 'frank',
              finisherID: 'frank',
              finishTime: '2025/02/18',
            },
            {
              orderID: 'I002',
              type: '版本迭代',
              startTime: '2025/02/01',
              finishTime: '2025/02/20',
              coordinationSheet: false,
              apiChanged: true,
              updateNotes: '增加地面操作日志功能',
              packingRequirement: '紧急封装',
              promoterID: 'gina',
              finisherID: 'gina',
              coordinationID: 'C002',
            },
          ],
        },
        {
          modelID: 'ATA08_WeightBalance',
          version: 'V3.0.0.0',
          updatetime: '2025-03-10',
          orders: [
            {
              orderID: 'F003',
              type: '功能开发',
              finishFeatureDesc: '自动称重与配载平衡优化算法。',
              promoterID: 'henry',
              finisherID: 'henry',
              finishTime: '2025/03/12',
            },
            {
              orderID: 'D003',
              type: '交付发送',
              targetCustomer: '重工',
              finishAuthId: 'AUTH-654321',
              finishShellNo: 'SHELL-003',
              promoterID: 'irene',
              finisherID: 'irene',
              finishTime: '2025/03/15',
              isCAEChecked: false,
              isEncrypted: true,
            },
            {
              orderID: 'I003',
              type: '版本迭代',
              startTime: '2025/03/01',
              finishTime: '2025/03/20',
              coordinationSheet: true,
              coordinationID: 'C003',
              apiChanged: false,
              updateNotes: '优化配载平衡算法',
              packingRequirement: '标准封装',
              promoterID: 'jack',
              finisherID: 'jack',
            },
          ],
        },
        {
          modelID: 'ATA21_AirConditioning',
          version: 'V4.0.0.0',
          updatetime: '2025-04-05',
          orders: [
            {
              orderID: 'F004',
              type: '功能开发',
              finishFeatureDesc: '智能空调温度调节与节能模式。',
              promoterID: 'kate',
              finisherID: 'kate',
              finishTime: '2025/04/06',
            },
            {
              orderID: 'R004',
              type: '问题复现',
              finishPhenomenon: '空调温度调节不灵敏',
              finishRemark: '需检查温度传感器',
              promoterID: 'leo',
              finisherID: 'leo',
              finishTime: '2025/04/08',
            },
            {
              orderID: 'I004',
              type: '版本迭代',
              startTime: '2025/04/01',
              finishTime: '2025/04/15',
              coordinationSheet: false,
              apiChanged: true,
              updateNotes: '新增节能模式',
              packingRequirement: '标准封装',
              promoterID: 'mona',
              finisherID: 'mona',
              coordinationID: 'C004',
            },
          ],
        },
        ...generateATA04Models(30), // 这里插入
      ]

      // 支持通过 query.modelID 过滤模型
      const modelID = query?.modelID
      if (modelID) {
        data = data.filter(item => item.modelID === modelID)
      }

      // 支持分页
      const page = Number(query?.page) || 1
      const pageSize = Number(query?.pageSize) || 10
      const total = data.length
      const list = data.slice((page - 1) * pageSize, page * pageSize)

      return {
        error: '',
        status: 1,
        data: {
          list,
          total,
        },
      }
    },
  },
  {
    url: '/mock/model/version-list',
    method: 'get',
    response: () => {
      // 可根据 query.modelID 返回不同数据
      return {
        status: 1,
        error: '',
        data: {
          list: [
            { version: '1.0.1.1C', updateNotes: '初始版本，支持基础功能' },
            { version: '1.0.1.2C', updateNotes: '修复若干bug，优化性能' },
            { version: '1.0.2.0C', updateNotes: '新增自动检测模块' },
            { version: '2.1.0.0C', updateNotes: '提升安全性，适配新平台' },
          ],
        },
      }
    },
  },
])
