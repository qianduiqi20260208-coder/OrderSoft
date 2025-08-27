// import { faker } from '@faker-js/faker'
import { defineFakeRoute } from 'vite-plugin-fake-server/client'

function getAdminOrders() {
  const types = ['问题复现', '版本迭代', '交付发送', '版本迭代+交付发送', '功能开发', '其他']
  const statusArr = ['草稿', '待审批', '待分发', '进行中', '已完成', '已退回']
  const orders: any[] = []
  for (let i = 1; i <= 150; i++) {
    const status = statusArr[i % statusArr.length]
    // 只有部分已完成工单有多次流转信息
    const transfers: any[] = []
    if (status === '已完成' || status === '进行中') {
      // 随机生成1~3次流转
      const transferCount = (i % 3) + 1
      for (let t = 0; t < transferCount; t++) {
        transfers.push({
          transferExecutorID: `transferExec${i}_${t + 1}`,
          transferReason: `流转原因示例${i}_${t + 1}`,
          transferTime: `2025/01/${String(i + 5 + t).padStart(2, '0')} 10:0${t}:00`,
        })
      }
    }
    orders.push({
      // 基础字段
      orderID: (2000 + i).toString(),
      type: types[i % types.length],
      status,
      referencePriority: i % 2 === 0 ? '紧急' : '一般',
      taskPriority: i % 2 === 0 ? '紧急' : '一般',
      modelID: `M${100 + i}`,
      modelName: 'AeroTest',
      modelVersionID: `V${i}.0.0`,
      promoterID: `user${i}`,
      startTime: `2025/01/${String(i).padStart(2, '0')}`,
      approverID: `leader${i}`,
      distributorID: `dis${i}`,
      approveTime: `2025/01/${String(i + 1).padStart(2, '0')}`,
      distributeTime: `2025/01/${String(i + 2).padStart(2, '0')}`,
      rejectReason: status === '已退回' ? `拒绝原因示例${i}` : '',
      executorID: `exec${i}`,
      finishTime: `2025/01/${String(i + 10).padStart(2, '0')}`,

      // 问题复现类
      coordinationID: `C${100 + i}`,
      description: `复现内容描述${i}`,
      files: [{ name: `file${i}.pdf`, url: `/files/file${i}.pdf` }],

      // 版本迭代类
      parentVersionID: `M${99 + i}`,
      updateNotes: `更新内容${i}`,
      packageRequirement: `要求${i}`,
      apiChanged: i % 2 === 0 ? '是' : '否',

      // 交付发送类
      targetCustomer: `客户${i}`,
      isEncrypted: i % 2 === 0 ? '是' : '否',
      isCAEChecked: i % 2 !== 0 ? '是' : '否',

      // 功能开发类
      featureDesc: `功能描述${i}`,

      // 其他类
      contentDesc: `内容描述${i}`,

      // 完成工单相关字段
      finishRemark: `备注${i}`,
      finishPhenomenon: `复现现象${i}`,
      finishModelVersion: `V${i}.0.1`,
      finishAuthId: `AUTH${i}`,
      finishShellNo: `SHELL${i}`,
      finishModelID: `M${200 + i}`,
      finishModelVersionId: `V${i}.0.2`,
      finishFeatureDesc: `完成功能描述${i}`,
      finishRemarkOther: `完成备注${i}`,

      // 多次流转相关字段（仅部分已完成工单有）
      transfers,
    })
  }
  // 新增六种类型的已完成工单
  orders.push(
    {
      orderID: '9001',
      type: '问题复现',
      status: '已完成',
      referencePriority: '紧急',
      taskPriority: '一般',
      modelID: 'M9001',
      modelName: 'AeroTest',
      modelVersionID: 'V9001.0.0',
      promoterID: 'user9001',
      startTime: '2025/07/22',
      approverID: 'leader9001',
      distributorID: 'dis9001',
      approveTime: '2025/07/23',
      distributeTime: '2025/07/24',
      executorID: 'exec9001',
      finishTime: '2025/07/30',
      coordinationID: 'C9001',
      description: '复现内容描述9001',
      files: [{ name: 'file9001.pdf', url: '/files/file9001.pdf' }],
      finishRemark: '备注9001',
      finishPhenomenon: '复现现象9001',
    },
    {
      orderID: '9002',
      type: '版本迭代',
      status: '已完成',
      referencePriority: '一般',
      taskPriority: '紧急',
      modelID: 'M9002',
      modelName: 'AeroTest',
      modelVersionID: 'V9002.0.0',
      promoterID: 'user9002',
      startTime: '2025/07/22',
      approverID: 'leader9002',
      distributorID: 'dis9002',
      approveTime: '2025/07/23',
      distributeTime: '2025/07/24',
      executorID: 'exec9002',
      finishTime: '2025/07/30',
      parentVersionID: 'M9001',
      updateNotes: '更新内容9002',
      packageRequirement: '要求9002',
      apiChanged: '否',
      finishModelVersion: 'V9002.0.1',
      finishRemark: '版本迭代完成',
    },
    {
      orderID: '9003',
      type: '交付发送',
      status: '已完成',
      referencePriority: '紧急',
      taskPriority: '一般',
      modelID: 'M9003',
      modelName: 'AeroTest',
      modelVersionID: 'V9003.0.0',
      promoterID: 'user9003',
      startTime: '2025/07/22',
      approverID: 'leader9003',
      distributorID: 'dis9003',
      approveTime: '2025/07/23',
      distributeTime: '2025/07/24',
      executorID: 'exec9003',
      finishTime: '2025/07/30',
      targetCustomer: '客户Z',
      isEncrypted: '是',
      isCAEChecked: '否',
      hasSensitiveInfo: '否',
      finishAuthId: 'AUTH9003',
      finishShellNo: 'SHELL9003',
      finishRemark: '交付完成',
    },
    {
      orderID: '9004',
      type: '版本迭代+交付发送',
      status: '已完成',
      referencePriority: '一般',
      taskPriority: '紧急',
      modelID: 'M9004',
      modelName: 'AeroTest',
      modelVersionID: 'V9004.0.0',
      promoterID: 'user9004',
      startTime: '2025/07/22',
      approverID: 'leader9004',
      distributorID: 'dis9004',
      approveTime: '2025/07/23',
      distributeTime: '2025/07/24',
      executorID: 'exec9004',
      finishTime: '2025/07/30',
      parentVersionID: 'M9003',
      updateNotes: '更新内容9004',
      packageRequirement: '要求9004',
      apiChanged: '是',
      targetCustomer: '客户Y',
      isEncrypted: '否',
      isCAEChecked: '是',
      hasSensitiveInfo: '是',
      finishModelVersion: 'V9004.0.1',
      finishAuthId: 'AUTH9004',
      finishShellNo: 'SHELL9004',
      finishRemark: '版本迭代+交付完成',
    },
    {
      orderID: '9005',
      type: '功能开发',
      status: '已完成',
      referencePriority: '紧急',
      taskPriority: '一般',
      modelID: 'M9005',
      modelName: 'AeroTest',
      modelVersionID: 'V9005.0.0',
      promoterID: 'user9005',
      startTime: '2025/07/22',
      approverID: 'leader9005',
      distributorID: 'dis9005',
      approveTime: '2025/07/23',
      distributeTime: '2025/07/24',
      executorID: 'exec9005',
      finishTime: '2025/07/30',
      featureDesc: '功能描述9005',
      finishModelID: 'M9005',
      finishModelVersionId: 'V9005.0.1',
      finishFeatureDesc: '完成功能开发9005',
    },
    {
      orderID: '9006',
      type: '其他',
      status: '已完成',
      referencePriority: '一般',
      taskPriority: '紧急',
      modelID: 'M9006',
      modelName: 'AeroTest',
      modelVersionID: 'V9006.0.0',
      promoterID: 'user9006',
      startTime: '2025/07/22',
      approverID: 'leader9006',
      distributorID: 'dis9006',
      approveTime: '2025/07/23',
      distributeTime: '2025/07/24',
      executorID: 'exec9006',
      finishTime: '2025/07/30',
      contentDesc: '内容描述9006',
      finishRemarkOther: '其他工单完成',
    },
  )

  return orders
}

// 客户列表 mock 数据
const mockCustomerList = ['客户A', '客户B', '客户C']

export default defineFakeRoute([
  {
    url: '/mock/order/list',
    method: 'get',
    response: (_req) => {
    // 手动返回几个模拟工单数据，字段更丰富
      const data = [
        {
          orderID: '1001',
          promoterID: 'userA',
          startTime: '2025-07-01 09:00:00',
          type: '问题复现',
          modelID: 'ATA04_气动模型',
          modelVersionID: 'v1.0.0',
          status: '待审批',
          approverID: '',
          approveTime: '',
          referencePriority: '',
          distributorID: '',
          distributeTime: '',
          taskPriority: '',
          finishTime: '',
          coordinationID: 'XJ-20250701',
          description: '模型在特定输入下无法复现预期结果',
          files: [{ name: '复现说明.docx' }],
        },
        {
          orderID: '1002',
          promoterID: 'userB',
          startTime: '2025-07-02 08:30:00',
          type: '功能开发',
          modelID: 'ATA04_气动模型',
          modelVersionID: 'v2.1.0',
          status: '进行中',
          approverID: 'leaderC',
          referencePriority: '一般',
          distributorID: 'leaderD',
          approveTime: '2025-07-02 09:00:00',
          taskPriority: '紧急',
          distributeTime: '2025-07-02 09:30:00',
          finishTime: '',
          executorID: 'userH',
          featureDesc: '新增自动化数据清洗功能',
        },
        {
          orderID: '1003',
          promoterID: 'userC',
          startTime: '2025-07-03 11:00:00',
          type: '交付发送',
          modelID: 'ATA04_气动模型',
          modelVersionID: 'v3.0.0',
          status: '已完成',
          approverID: 'leaderE',
          referencePriority: '一般',
          distributorID: 'leaderF',
          approveTime: '2025-07-03 12:00:00',
          taskPriority: '一般',
          distributeTime: '2025-07-03 12:30:00',
          finishTime: '2025-07-03 15:00:00',
          executorID: 'userG',
          targetCustomer: '客户A',
          isEncrypted: '是',
          isCAEChecked: '否',
        },
        {
          orderID: '1004',
          promoterID: 'userD',
          startTime: '2025-07-04 14:00:00',
          type: '版本迭代',
          modelID: 'ATA04_气动模型',
          modelVersionID: 'v4.2.0',
          status: '待分发',
          approverID: 'leaderG',
          approveTime: '2025-07-04 15:00:00',
          referencePriority: '紧急',
          distributorID: '',
          taskPriority: '',
          distributeTime: '',
          finishTime: '',
          coordinationID: 'NA',
          parentVersionID: 'v4.1.0',
          updateNotes: '修复了数据同步bug，优化了性能',
          packageRequirement: '需支持Windows和Linux双平台',
          apiChanged: '否',
        },
        {
          orderID: '1006',
          promoterID: 'userF',
          startTime: '2025-07-06 13:00:00',
          type: '版本迭代+交付发送',
          modelID: 'ATA04_气动模型',
          modelVersionID: 'v6.0.0',
          status: '已完成',
          approverID: 'leaderH',
          approveTime: '2025-07-06 14:00:00',
          referencePriority: '紧急',
          distributorID: 'leaderI',
          taskPriority: '紧急',
          distributeTime: '2025-07-06 14:30:00',
          finishTime: '2025-07-06 17:00:00',
          executorID: 'userJ',
          coordinationID: 'XJ-20250706',
          parentVersionID: 'v5.9.0',
          updateNotes: '迭代新算法，提升准确率',
          packageRequirement: '需支持Mac平台',
          apiChanged: '是',
          targetCustomer: '客户B',
          isEncrypted: '否',
          isCAEChecked: '是',
        },
        {
          orderID: '2001',
          promoterID: 'userX1',
          startTime: '2025-07-10 10:00:00',
          type: '问题复现',
          modelID: 'ATA05_结构模型',
          modelVersionID: 'v1.2.0',
          status: '进行中',
          approverID: 'leaderX1',
          approveTime: '2025-07-10 10:30:00',
          referencePriority: '一般',
          distributorID: 'leaderX2',
          distributeTime: '2025-07-10 11:00:00',
          taskPriority: '一般',
          finishTime: '',
          coordinationID: 'XJ-20250710',
          description: '结构模型边界条件异常',
          files: [{ name: '结构说明.pdf' }],
        },
        {
          orderID: '2002',
          promoterID: 'userX2',
          startTime: '2025-07-11 09:00:00',
          type: '功能开发',
          modelID: 'ATA06_热力模型',
          modelVersionID: 'v3.0.0',
          status: '进行中',
          approverID: 'leaderX3',
          approveTime: '2025-07-11 09:30:00',
          referencePriority: '紧急',
          distributorID: 'leaderX4',
          distributeTime: '2025-07-11 10:00:00',
          taskPriority: '紧急',
          finishTime: '',
          executorID: 'userX5',
          featureDesc: '开发热流分析自动报告功能',
        },
        {
          orderID: '2003',
          promoterID: 'userX3',
          startTime: '2025-07-12 08:00:00',
          type: '交付发送',
          modelID: 'ATA07_动力模型',
          modelVersionID: 'v2.5.1',
          status: '进行中',
          approverID: 'leaderX5',
          approveTime: '2025-07-12 08:30:00',
          referencePriority: '一般',
          distributorID: 'leaderX6',
          distributeTime: '2025-07-12 09:00:00',
          taskPriority: '一般',
          finishTime: '',
          executorID: 'userX6',
          targetCustomer: '客户D',
          isEncrypted: '否',
          isCAEChecked: '是',
          hasSensitiveInfo: '否',
        },
        {
          orderID: '2004',
          promoterID: 'userX4',
          startTime: '2025-07-13 14:00:00',
          type: '版本迭代',
          modelID: 'ATA08_控制模型',
          modelVersionID: 'v5.1.0',
          status: '进行中',
          approverID: 'leaderX7',
          approveTime: '2025-07-13 14:30:00',
          referencePriority: '紧急',
          distributorID: 'leaderX8',
          distributeTime: '2025-07-13 15:00:00',
          taskPriority: '紧急',
          finishTime: '',
          coordinationID: 'XJ-20250713',
          parentVersionID: 'v5.0.0',
          updateNotes: '优化控制算法，提升响应速度',
          packageRequirement: '支持多平台部署',
          apiChanged: '是',
        },
        {
          orderID: '2005',
          promoterID: 'userX5',
          startTime: '2025-07-14 13:00:00',
          type: '版本迭代+交付发送',
          modelID: 'ATA09_仿真模型',
          modelVersionID: 'v7.0.0',
          status: '进行中',
          approverID: 'leaderX9',
          approveTime: '2025-07-14 13:30:00',
          referencePriority: '一般',
          distributorID: 'leaderX10',
          distributeTime: '2025-07-14 14:00:00',
          taskPriority: '一般',
          finishTime: '',
          executorID: 'userX7',
          coordinationID: 'XJ-20250714',
          parentVersionID: 'v6.9.0',
          updateNotes: '仿真精度提升，新增数据导出功能',
          packageRequirement: '支持云端部署',
          apiChanged: '否',
          targetCustomer: '客户E',
          isEncrypted: '是',
          isCAEChecked: '否',
          hasSensitiveInfo: '是',
        },
        {
          orderID: '2006',
          promoterID: 'userX6',
          startTime: '2025-07-15 15:00:00',
          type: '其他',
          modelID: 'ATA10_测试模型',
          modelVersionID: 'v0.9.0',
          status: '进行中',
          approverID: 'leaderX11',
          approveTime: '2025-07-15 15:30:00',
          referencePriority: '一般',
          distributorID: 'leaderX12',
          distributeTime: '2025-07-15 16:00:00',
          taskPriority: '一般',
          finishTime: '',
          executorID: 'userX8',
          contentDesc: '测试模型数据补录及文档完善',
        },
        {
          orderID: '3001',
          promoterID: 'userZ1',
          startTime: '2025-07-20 09:00:00',
          type: '问题复现',
          modelID: 'ATA20_气动模型',
          modelVersionID: 'v2.0.0',
          status: '进行中', // 修改
          approverID: 'leaderZ1',
          approveTime: '2025-07-20 09:30:00',
          referencePriority: '紧急',
          distributorID: 'leaderZ2',
          distributeTime: '2025-07-20 10:00:00',
          taskPriority: '一般',
          finishTime: '', // 修改
          executorID: 'userZ2',
          coordinationID: 'XJ-20250720',
          description: '气动模型多次流转测试',
          files: [{ name: '气动流转.pdf' }],
          finishPhenomenon: '流转现象正常',
          finishRemark: '多次流转无异常',
          transfers: [
            {
              transferExecutorID: 'execZ1',
              transferReason: '第一次流转-需补充数据',
              transferTime: '2025/07/20 10:30:00',
            },
            {
              transferExecutorID: 'execZ2',
              transferReason: '第二次流转-需专家复核',
              transferTime: '2025/07/20 11:00:00',
            },
            {
              transferExecutorID: 'execZ3',
              transferReason: '第三次流转-最终确认',
              transferTime: '2025/07/20 11:30:00',
            },
          ],
        },
        {
          orderID: '3002',
          promoterID: 'userZ3',
          startTime: '2025-07-21 10:00:00',
          type: '版本迭代',
          modelID: 'ATA21_结构模型',
          modelVersionID: 'v3.0.0',
          status: '进行中', // 修改
          approverID: 'leaderZ3',
          approveTime: '2025-07-21 10:30:00',
          referencePriority: '一般',
          distributorID: 'leaderZ4',
          distributeTime: '2025-07-21 11:00:00',
          taskPriority: '紧急',
          finishTime: '', // 修改
          executorID: 'userZ4',
          coordinationID: 'XJ-20250721',
          parentVersionID: 'v2.9.0',
          updateNotes: '结构模型多次流转测试',
          packageRequirement: '支持多平台',
          apiChanged: '否',
          finishModelVersion: 'v3.0.1',
          finishRemark: '多次流转已完成',
          transfers: [
            {
              transferExecutorID: 'execZ4',
              transferReason: '第一次流转-接口调整',
              transferTime: '2025/07/21 11:30:00',
            },
            {
              transferExecutorID: 'execZ5',
              transferReason: '第二次流转-性能优化',
              transferTime: '2025/07/21 12:00:00',
            },
          ],
        },
        {
          orderID: '3003',
          promoterID: 'userZ5',
          startTime: '2025-07-22 11:00:00',
          type: '交付发送',
          modelID: 'ATA22_热力模型',
          modelVersionID: 'v4.0.0',
          status: '进行中', // 修改
          approverID: 'leaderZ5',
          approveTime: '2025-07-22 11:30:00',
          referencePriority: '紧急',
          distributorID: 'leaderZ6',
          distributeTime: '2025-07-22 12:00:00',
          taskPriority: '一般',
          finishTime: '', // 修改
          executorID: 'userZ6',
          targetCustomer: '客户Z',
          isEncrypted: '是',
          isCAEChecked: '是',
          hasSensitiveInfo: '否',
          finishAuthId: 'AUTHZ1',
          finishShellNo: 'SHELLZ1',
          finishRemark: '交付多次流转',
          transfers: [
            {
              transferExecutorID: 'execZ7',
              transferReason: '第一次流转-客户需求变更',
              transferTime: '2025/07/22 12:30:00',
            },
            {
              transferExecutorID: 'execZ8',
              transferReason: '第二次流转-补充授权',
              transferTime: '2025/07/22 13:00:00',
            },
          ],
        },
      ]
      return {
        error: '',
        status: 1,
        data: {
          list: data,
          total: data.length,
        },
      }
    },
  },
  {
    // 获取当前用户近期发送工单 - 为主页专门添加
    url: '/mock/order/recent',
    method: 'get',
    response: ({ query }) => {
      const { userID } = query

      // 简化的5条近期发送工单mock数据
      const recentOrders = [
        {
          orderID: '2021',
          type: '问题复现',
          status: '已完成',
          referencePriority: '紧急',
          taskPriority: '一般',
          modelID: 'ATA04_气动模型',
          modelVersionID: 'v1.2.0',
          promoterID: userID || 'userA',
          startTime: '2025-08-05 10:30:00',
        },
        {
          orderID: '2022',
          type: '版本迭代',
          status: '进行中',
          referencePriority: '一般',
          taskPriority: '紧急',
          modelID: 'ATA05_结构模型',
          modelVersionID: 'v2.1.0',
          promoterID: userID || 'userA',
          startTime: '2025-08-04 14:20:00',
        },
        {
          orderID: '2023',
          type: '交付发送',
          status: '待审批',
          referencePriority: '紧急',
          taskPriority: '紧急',
          modelID: 'ATA06_控制模型',
          modelVersionID: 'v1.0.0',
          promoterID: userID || 'userA',
          startTime: '2025-08-03 09:15:00',
        },
        {
          orderID: '2024',
          type: '功能开发',
          status: '已退回',
          referencePriority: '一般',
          taskPriority: '一般',
          modelID: 'ATA07_仿真模型',
          modelVersionID: 'v3.0.1',
          promoterID: userID || 'userA',
          startTime: '2025-08-02 16:45:00',
        },
        {
          orderID: '2025',
          type: '版本迭代+交付发送',
          status: '已完成',
          referencePriority: '一般',
          taskPriority: '一般',
          modelID: 'ATA08_优化模型',
          modelVersionID: 'v1.5.0',
          promoterID: userID || 'userA',
          startTime: '2025-08-01 11:20:00',
        },
      ]

      return {
        error: '',
        status: 1,
        data: {
          list: recentOrders,
          total: recentOrders.length,
        },
      }
    },
  },
  {
    url: '/mock/order/all',
    method: 'get',
    response: ({ query, headers }) => {
      let data: any[] = []
      let total = 0

      // 获取筛选参数
      const filterOrderID = query.orderID || ''
      const filterType = query.type || ''
      const filterPromoterID = query.promoterID || ''
      const filterModelID = query.modelID || ''
      const filterReferencePriority = query.referencePriority || ''
      const filterTaskPriority = query.taskPriority || ''
      const filterStatus = query.status || ''
      const filterStartDate = query.startDate || ''
      const filterEndDate = query.endDate || ''

      if (headers.token?.indexOf('admin') === 0) {
        data = getAdminOrders()
        // 工单ID精确或模糊匹配
        if (filterOrderID) {
          data = data.filter(item => item.orderID.includes(filterOrderID))
        }
        // 工单类型筛选
        if (filterType) {
          data = data.filter(item => item.type === filterType)
        }
        // 员工工号筛选
        if (filterPromoterID) {
          data = data.filter(item => item.promoterID === filterPromoterID)
        }
        // 模型筛选
        if (filterModelID) {
          data = data.filter(item => item.modelID === filterModelID)
        }
        // 优先级筛选（参考优先级和任务优先级分别处理）
        if (filterReferencePriority) {
          data = data.filter(item => item.referencePriority === filterReferencePriority)
        }
        if (filterTaskPriority) {
          data = data.filter(item => item.taskPriority === filterTaskPriority)
        }
        // 状态筛选
        if (filterStatus) {
          data = data.filter(item => item.status === filterStatus)
        }
        // 日期区间筛选（发起时间）
        if (filterStartDate && filterEndDate) {
          data = data.filter((item) => {
            const t = new Date(item.startTime.replace(/-/g, '/')).getTime()
            const startDateStr = Array.isArray(filterStartDate) ? filterStartDate[0] : filterStartDate
            const endDateStr = Array.isArray(filterEndDate) ? filterEndDate[0] : filterEndDate
            return t >= new Date(startDateStr).getTime() && t <= new Date(endDateStr).getTime()
          })
        }
        total = data.length
      }
      else if (headers.token?.indexOf('test') === 0) {
      // ...原 test 角色数据和筛选逻辑同上，可按需补充...
        data = [
        // ...测试数据...
        ]
        if (filterOrderID) {
          data = data.filter(item => item.orderID.includes(filterOrderID))
        }
        if (filterType) {
          data = data.filter(item => item.type === filterType)
        }
        if (filterPromoterID) {
          data = data.filter(item => item.promoterID === filterPromoterID)
        }
        if (filterModelID) {
          data = data.filter(item => item.modelID === filterModelID)
        }
        if (filterReferencePriority) {
          data = data.filter(item => item.referencePriority === filterReferencePriority)
        }
        if (filterTaskPriority) {
          data = data.filter(item => item.taskPriority === filterTaskPriority)
        }
        if (filterStatus) {
          data = data.filter(item => item.status === filterStatus)
        }
        if (filterStartDate && filterEndDate) {
          data = data.filter((item) => {
            const t = new Date(item.startTime.replace(/-/g, '/')).getTime()
            const startDateStr = Array.isArray(filterStartDate) ? filterStartDate[0] : filterStartDate
            const endDateStr = Array.isArray(filterEndDate) ? filterEndDate[0] : filterEndDate
            return t >= new Date(startDateStr).getTime() && t <= new Date(endDateStr).getTime()
          })
        }
        total = data.length
      }
      // 其它角色可继续补充

      const page = Number.parseInt(Array.isArray(query.page) ? query.page[0] : query.page || '1')
      const pageSize = Number.parseInt(Array.isArray(query.pageSize) ? query.pageSize[0] : query.pageSize || '10')
      const start = (page - 1) * pageSize
      const end = start + pageSize
      const pageData = data.slice(start, end)
      return {
        error: '',
        status: 1,
        data: {
          list: pageData,
          total,
          page,
          pageSize,
        },
      }
    },
  },

  // 新增：根据工单号查询单个工单
  {
    url: '/mock/order/detail',
    method: 'get',
    response: ({ query }) => {
      const { orderNumber } = query

      if (!orderNumber) {
        return {
          error: '工单号不能为空',
          status: 0,
          data: null,
        }
      }

      // 模拟工单数据（这里应该包含所有可能的工单数据）
      const mockOrders = [
        {
          orderID: 'WO202507310001',
          type: '交付发送',
          status: '已完成',
          referencePriority: '紧急',
          taskPriority: '紧急',
          modelID: 'ATA04_Aerodynamics',
          modelVersionID: 'V3.2.1.2C',
          promoterID: 'userA',
          startTime: '2025-07-31 08:30:15',
          approverID: 'userB',
          distributorID: 'userC',
          approveTime: '2025-07-31 09:00:00',
          distributeTime: '2025-07-31 09:30:00',
          executorID: 'userD',
          finishTime: '2025-07-31 18:00:00',
          targetCustomer: 'CAE技术有限公司',
          isCAEChecked: '是',
          hasSensitiveInfo: '否',
          isEncrypted: '是',
          finishAuthId: 'AUTH123456',
          finishShellNo: 'SHELL789ABC',
          finishRemark: '交付发送完成，客户已确认收到',
        },
        {
          orderID: 'WO202507310002',
          type: '版本迭代',
          status: '进行中',
          referencePriority: '一般',
          taskPriority: '一般',
          modelID: 'ATA04_Aerodynamics',
          modelVersionID: 'V3.2.1.2C',
          promoterID: 'userB',
          startTime: '2025-07-31 10:45:30',
          approverID: 'userA',
          distributorID: 'userC',
          approveTime: '2025-07-31 11:00:00',
          distributeTime: '2025-07-31 11:30:00',
          executorID: 'userD',
          coordinationID: 'COORD-001',
          updateNotes: '优化空气动力学计算算法，提升计算精度',
          packageRequirement: '标准封装，包含所有依赖库',
          apiChanged: '否',
        },
        // ... 更多工单数据
      ]

      // 查找匹配的工单
      const foundOrder = mockOrders.find(order => order.orderID === orderNumber)

      if (foundOrder) {
        return {
          error: '',
          status: 1,
          data: foundOrder,
        }
      }
      else {
        return {
          error: '工单不存在',
          status: 1,
          data: null,
        }
      }
    },
  },

  // 提交问题复现工单 模拟文件
  {
    url: '/mock/order/problem',
    method: 'post',
    response: ({ body }) => {
      // body: 普通字段，body.files: 文件数组（模拟FormData上传）
      const fileInfos = (body.files || []).map((file: any) => ({
        name: file.filename || file.name || '未知文件',
        size: file.size || 0,
        url: `/mock/files/${file.filename || file.name || Date.now()}`,
      }))
      return {
        error: '',
        status: 1,
        data: {
          orderID: body.orderID || `GX-${Date.now()}`,
          status: body.status,
          promoterID: body.promoterID,
          modelID: body.modelID,
          modelVersionID: body.modelVersionID,
          coordinationID: body.coordinationID || '',
          description: body.description,
          approverID: body.approverID,
          startTime: body.startTime,
          files: fileInfos,
          message: '问题复现工单提交成功',
        },
      }
    },
  },

  // 提交版本迭代工单
  {
    url: '/mock/order/iter',
    method: 'post',
    response: ({ body }) => {
    // body 就是提交的版本迭代工单数据
      return {
        error: '',
        status: 1,
        data: {
          orderID: body.orderID || `ITER-${Date.now()}`, // 若无则自动生成一个ID
          status: body.status,
          promoterID: body.promoterID,
          modelID: body.modelID,
          modelVersionID: body.modelVersionID,
          coordinationID: body.coordinationID || '',
          updateNotes: body.updateNotes,
          packageRequirement: body.packageRequirement || '',
          apiChanged: body.apiChanged,
          approverID: body.approverID,
          startTime: body.startTime,
          // 可补充其它默认字段
          message: '版本迭代工单提交成功',
        },
      }
    },
  },
  // 提交交付发送工单
  {
    url: '/mock/order/deliver',
    method: 'post',
    response: ({ body }) => {
    // body 就是提交的交付发送工单数据
      return {
        error: '',
        status: 1,
        data: {
          orderID: body.orderID || `DELIVER-${Date.now()}`, // 若无则自动生成一个ID
          status: body.status,
          promoterID: body.promoterID,
          modelID: body.modelID,
          modelVersionID: body.modelVersionID,
          targetCustomer: body.targetCustomer,
          hasSensitiveInfo: body.hasSensitiveInfo || false, // 是否包含敏感信息
          approverID: body.approverID,
          startTime: body.startTime,
          // 可补充其它默认字段
          message: '交付发送工单提交成功',
        },
      }
    },
  },
  // 提交版本迭代+交付发送工单
  {
    url: '/mock/order/iter-deliver',
    method: 'post',
    response: ({ body }) => {
    // body 就是提交的模型迭代+交付发送工单数据
      return {
        error: '',
        status: 1,
        data: {
          orderID: body.orderID || `ITERDELIVER-${Date.now()}`, // 若无则自动生成一个ID
          status: body.status,
          promoterID: body.promoterID,
          modelID: body.modelID,
          modelVersionID: body.modelVersionID,
          coordinationID: body.coordinationID || '',
          updateNotes: body.updateNotes,
          packageRequirement: body.packageRequirement || '',
          apiChanged: body.apiChanged,
          targetCustomer: body.targetCustomer,
          isCAEChecked: body.isCAEChecked,
          hasSensitiveInfo: body.hasSensitiveInfo || false, // 是否包含敏感信息
          startTime: body.startTime,
          approverID: body.approverID,
          // 可补充其它默认字段
          message: '版本迭代+交付发送工单提交成功',
        },
      }
    },
  },
  // 提交功能开发工单
  {
    url: '/mock/order/dev',
    method: 'post',
    response: ({ body }) => {
    // body 就是提交的功能开发工单数据
      return {
        error: '',
        status: 1,
        data: {
          orderID: body.orderID || `DEV-${Date.now()}`, // 若无则自动生成一个ID
          status: body.status,
          promoterID: body.promoterID,
          modelID: body.modelID,
          modelVersionID: body.modelVersionID,
          featureDesc: body.featureDesc,
          startTime: body.startTime,
          approverID: body.approverID,
          // 可补充其它默认字段
          message: '功能开发工单提交成功',
        },
      }
    },
  },
  // 提交其他工单
  {
    url: '/mock/order/other',
    method: 'post',
    response: ({ body }) => {
    // body 就是提交的其他工单数据
      return {
        error: '',
        status: 1,
        data: {
          orderID: body.orderID || `OTHER-${Date.now()}`, // 若无则自动生成一个ID
          status: body.status,
          promoterID: body.promoterID,
          modelID: body.modelID,
          modelVersionID: body.modelVersionID,
          contentDesc: body.contentDesc,
          startTime: body.startTime,
          approverID: body.approverID,
          // 可补充其它默认字段
          message: '其他类工单提交成功',
        },
      }
    },
  },
  // 审批同意
  {
    url: '/mock/order/approve',
    method: 'post',
    response: ({ body }) => {
    // body 就是审批工单提交的数据
      return {
        error: '',
        status: 1,
        data: {
          orderID: body.orderID, // 工单ID
          status: body.status, // 工单状态（审批后变为待分发）
          approverID: body.approverID, // 审批人ID
          approveTime: body.approveTime, // 审批通过时间
          referencePriority: body.referencePriority, // 参考优先级
          distributorID: body.distributorID, // 下一流程负责人ID
          message: '工单审批通过，状态已更新为待分发',
        },
      }
    },
  },
  // 审批拒绝
  {
    url: '/mock/order/reject-approve',
    method: 'post',
    response: ({ body }) => {
    // 审批环节拒绝
      return {
        error: '',
        status: 1,
        data: {
          orderID: body.orderID, // 工单ID
          status: body.status, // 工单状态（已退回）
          approverID: body.approverID, // 审批人ID
          approveTime: body.approveTime, // 审批时间
          rejectReason: body.rejectReason, // 拒绝原因
          message: '审批环节已退回工单',
        },
      }
    },
  },
  // 分发通过
  {
    url: '/mock/order/distribute',
    method: 'post',
    response: ({ body }) => {
    // body 就是分发工单提交的数据
      return {
        error: '',
        status: 1,
        data: {
          orderID: body.orderID, // 工单ID
          status: body.status, // 工单状态（分发后变为进行中）
          distributorID: body.distributorID, // 分发人ID
          distributeTime: body.distributeTime, // 分发时间
          taskPriority: body.taskPriority, // 任务优先级
          executorID: body.executorID, // 执行人ID
          message: '工单分发成功，状态已更新为进行中',
        },
      }
    },
  },
  // 分发拒绝
  {
    url: '/mock/order/reject-distribute',
    method: 'post',
    response: ({ body }) => {
    // 分发环节拒绝
      return {
        error: '',
        status: 1,
        data: {
          orderID: body.orderID, // 工单ID
          status: body.status, // 工单状态（已退回）
          distributorID: body.distributorID, // 分发人ID
          distributeTime: body.distributeTime, // 分发时间
          rejectReason: body.rejectReason, // 拒绝原因
          message: '分发环节已退回工单',
        },
      }
    },
  },
  // 问题复现工单完成 mock
  {
    url: '/mock/order/finish-problem',
    method: 'post',
    response: ({ body }) => {
    // 问题复现工单完成，返回工单ID、状态、完成时间、现象、备注、完成人ID等
      return {
        error: '',
        status: 1,
        data: {
          orderID: body.orderID, // 工单ID
          status: body.status, // 工单状态（已完成）
          finishTime: body.finishTime, // 完成时间
          finishPhenomenon: body.finishPhenomenon, // 复现现象
          finishRemark: body.finishRemark, // 备注
          executorID: body.executorID, // 完成人ID
          message: '问题复现工单已完成',
        },
      }
    },
  },

  // 版本迭代工单完成 mock
  {
    url: '/mock/order/finish-iter',
    method: 'post',
    response: ({ body }) => {
    // 版本迭代工单完成，返回工单ID、状态、完成时间、模型版本、备注、完成人ID等
      return {
        error: '',
        status: 1,
        data: {
          orderID: body.orderID,
          status: body.status,
          finishTime: body.finishTime,
          finishModelVersion: body.finishModelVersion,
          finishRemark: body.finishRemark,
          executorID: body.executorID,
          message: '版本迭代工单已完成',
        },
      }
    },
  },

  // 交付发送工单完成 mock
  {
    url: '/mock/order/finish-deliver',
    method: 'post',
    response: ({ body }) => {
    // 交付发送工单完成，返回工单ID、状态、完成时间、授权ID、外壳号、备注、完成人ID等
      return {
        error: '',
        status: 1,
        data: {
          orderID: body.orderID,
          status: body.status,
          finishTime: body.finishTime,
          isEncrypted: body.isEncrypted,
          finishAuthId: body.finishAuthId,
          finishShellNo: body.finishShellNo,
          finishRemark: body.finishRemark,
          executorID: body.executorID,
          message: '交付发送工单已完成',
        },
      }
    },
  },

  // 版本迭代+交付发送工单完成 mock
  {
    url: '/mock/order/finish-iter-deliver',
    method: 'post',
    response: ({ body }) => {
    // 版本迭代+交付发送工单完成，返回工单ID、状态、完成时间、模型版本、授权ID、外壳号、备注、完成人ID等
      return {
        error: '',
        status: 1,
        data: {
          orderID: body.orderID,
          status: body.status,
          finishTime: body.finishTime,
          finishModelVersion: body.finishModelVersion,
          isEncrypted: body.isEncrypted,
          finishAuthId: body.finishAuthId,
          finishShellNo: body.finishShellNo,
          finishRemark: body.finishRemark,
          executorID: body.executorID,
          message: '版本迭代+交付发送工单已完成',
        },
      }
    },
  },

  // 功能开发工单完成 mock
  {
    url: '/mock/order/finish-dev',
    method: 'post',
    response: ({ body }) => {
    // 功能开发工单完成，返回工单ID、状态、完成时间、模型ID、模型版本ID、功能描述、完成人ID等
      return {
        error: '',
        status: 1,
        data: {
          orderID: body.orderID,
          status: body.status,
          finishTime: body.finishTime,
          finishModelID: body.finishModelID,
          finishModelVersionId: body.finishModelVersionId,
          finishFeatureDesc: body.finishFeatureDesc,
          executorID: body.executorID,
          message: '功能开发工单已完成',
        },
      }
    },
  },

  // 其他工单完成 mock
  {
    url: '/mock/order/finish-other',
    method: 'post',
    response: ({ body }) => {
    // 其他工单完成，返回工单ID、状态、完成时间、备注、完成人ID等
      return {
        error: '',
        status: 1,
        data: {
          orderID: body.orderID,
          status: body.status,
          finishTime: body.finishTime,
          finishRemarkOther: body.finishRemarkOther,
          executorID: body.executorID,
          message: '其他工单已完成',
        },
      }
    },
  },

  // 工单流转 mock
  {
    url: '/mock/order/transfer',
    method: 'post',
    response: ({ body }) => {
    // 工单流转，返回工单ID、流转执行人ID、流转原因、流转时间等
      return {
        error: '',
        status: 1,
        data: {
          orderID: body.orderID,
          transferExecutorID: body.transferExecutorID,
          transferReason: body.transferReason,
          transferTime: body.transferTime,
          message: '工单流转信息已提交',
        },
      }
    },
  },

  // 根据模型id返回对应的模型版本id
  {
    url: '/mock/model/version/list',
    method: 'get',
    response: ({ query }) => {
      // 根据modelId返回不同的版本列表
      let list: string[] = []
      if (query.modelId === 'ATA04') {
      // 生成30条示例数据
        list = Array.from({ length: 30 }, (_, i) => `V2.0.${Math.floor(i / 5) + 1}.${(i % 5) + 1}C`)
      }
      else if (query.modelId === 'ATA05') {
        list = ['V3.1.0.1A', 'V3.1.0.2B']
      }
      else {
        list = ['V1.0.0.1A']
      }
      return {
        error: '',
        status: 1,
        data: { list },
      }
    },
  },

  // 获取审批人列表
  {
    url: '/mock/approver/list',
    method: 'get',
    response: () => {
      return {
        error: '',
        status: 1,
        data: {
          list: [
            '10001',
            '10002',
            '10003',
            '10004',
            '10005',
          ],
        },
      }
    },
  },

  // 获取客户列表
  {
    url: '/mock/customer/list',
    method: 'get',
    response: () => {
      return {
        error: '',
        status: 1,
        data: {
          list: mockCustomerList,
        },
      }
    },
  },

  // 新增客户
  {
    url: '/mock/customer/add',
    method: 'post',
    response: ({ body }) => {
      const name = body?.name
      if (name && !mockCustomerList.includes(name)) {
        mockCustomerList.push(name)
        return {
          error: '',
          status: 1,
          data: {
            success: true,
            message: '添加成功',
            name,
          },
        }
      }
      return {
        error: '',
        status: 1,
        data: {
          success: false,
          message: '客户已存在或名称无效',
        },
      }
    },
  },
  // 添加流转负责人列表的Mock数据
  {
    url: '/mock/transferExecutor/list',
    method: 'get',
    response: () => {
      return {
        error: '',
        status: 1,
        data: {
          list: [
            'alice',
            'bob',
            'carol',
            'dave',
            'eva',
            'frank',
            'gina',
            'henry',
            'irene',
            'jack',
            'kate',
            'leo',
            'mona',
            'nina',
            'oliver',
            'paul',
            'quinn',
            'ruby',
          ],
        },
      }
    },
  },

  // 添加外壳号和授权ID列表的Mock数据
  {
    url: '/mock/order/shell-numbers',
    method: 'get',
    response: () => {
      return {
        error: '',
        status: 1,
        data: {
          list: [
            'A5300000000001',
            'A5300000000002',
            'A5300000000003',
            'A5300000000004',
            'A5300000000005',
            'A5300000000006',
            'A5300000000007',
            'A5300000000008',
            'A5300000000009',
            'A5300000000010',
            'A5300000000011',
            'A5300000000012',
          ],
        },
      }
    },
  },

  {
    url: '/mock/order/customer-auth-ids',
    method: 'get',
    response: () => {
      return {
        error: '',
        status: 1,
        data: {
          list: [
            {
              authId: '2024082872',
              shellNumberList: [
                {
                  description: '限时授权',
                  deviceType: '本地锁',
                  endTime: '2024-11-08',
                  shellNumber: 'A53000000004',
                  startTime: '2024-08-28',
                },
                {
                  description: 'FFS#10 限时授权',
                  deviceType: '本地锁',
                  endTime: '2024-11-08',
                  shellNumber: 'A53000000005',
                  startTime: '2024-08-28',
                },
              ],
            },
            {
              authId: '2024120372',
              shellNumberList: [
                {
                  description: 'FTD',
                  deviceType: '本地锁',
                  endTime: '2025-07-07',
                  shellNumber: 'A53000000004',
                  startTime: '2024-12-06',
                },
                {
                  description: 'FFS#10',
                  deviceType: '本地锁',
                  endTime: '2025-03-13',
                  shellNumber: 'A53000000005',
                  startTime: '2024-12-06',
                },
              ],
            },
            {
              authId: '2025011472',
              shellNumberList: [
                {
                  description: '限时授权',
                  deviceType: '本地锁',
                  endTime: '2025-03-13',
                  shellNumber: 'A53000000005',
                  startTime: '2025-02-13',
                },
              ],
            },
            {
              authId: '2025022672',
              shellNumberList: [
                {
                  description: 'FFS 发动模型采用网络锁',
                  deviceType: '网络锁',
                  endTime: '2025-03-27',
                  shellNumber: 'A53000000005',
                  startTime: '2025-02-27',
                },
                {
                  description: 'FFS10集成测试用',
                  deviceType: '网络锁',
                  endTime: '2025-03-27',
                  shellNumber: 'A53000000006',
                  startTime: '2025-03-13',
                },
              ],
            },
            {
              authId: '2025032672',
              shellNumberList: [
                {
                  description: 'FTD 使用',
                  deviceType: '本地锁',
                  endTime: '2025-10-26',
                  shellNumber: 'A53000000004',
                  startTime: '2025-03-26',
                },
                {
                  description: 'FFS 发送机网络授权',
                  deviceType: '网络锁',
                  endTime: '2025-12-31',
                  shellNumber: 'A53000000005',
                  startTime: '2025-03-26',
                },
                {
                  description: '本地测试',
                  deviceType: '本地锁',
                  endTime: '2025-08-31',
                  shellNumber: 'A53000000006',
                  startTime: '2025-03-26',
                },
              ],
            },
          ],
        },
      }
    },
  },

  // 新增：工单统计数据接口
  {
    url: '/mock/order/statistics',
    method: 'get',
    response: () => {
      // 模拟客户列表 - 只包含CAE和华模科技
      const clients = ['CAE', '华模科技']
      const today = new Date()

      // 生成近7天数据（按天）
      interface DailyOrderStat {
        date: string
        clientName: string
        versionIterationCount: number
        deliveryCount: number
      }
      const dailyData: DailyOrderStat[] = []
      for (let i = 6; i >= 0; i--) {
        const date = new Date(today)
        date.setDate(date.getDate() - i)
        const dateStr = date.toISOString().split('T')[0]

        clients.forEach((client) => {
          dailyData.push({
            date: dateStr,
            clientName: client,
            versionIterationCount: client === 'CAE' ? Math.floor(Math.random() * 2) + 1 : Math.floor(Math.random() * 3) + 2, // CAE: 1-2个, 华模: 2-4个
            deliveryCount: client === 'CAE' ? Math.floor(Math.random() * 3) + 2 : Math.floor(Math.random() * 4) + 3, // CAE: 2-4个, 华模: 3-6个
          })
        })
      }

      // 生成近1月数据（按周）
      interface WeeklyOrderStat {
        weekLabel: string
        clientName: string
        versionIterationCount: number
        deliveryCount: number
      }
      const weeklyData: WeeklyOrderStat[] = []
      for (let i = 3; i >= 0; i--) {
        const weekNum = 4 - i
        const weekLabel = weekNum === 4 ? '第4周(本周)' : `第${weekNum}周`

        clients.forEach((client) => {
          weeklyData.push({
            weekLabel,
            clientName: client,
            versionIterationCount: client === 'CAE' ? Math.floor(Math.random() * 8) + 5 : Math.floor(Math.random() * 10) + 8, // CAE: 5-12个, 华模: 8-17个
            deliveryCount: client === 'CAE' ? Math.floor(Math.random() * 12) + 8 : Math.floor(Math.random() * 15) + 12, // CAE: 8-19个, 华模: 12-26个
          })
        })
      }

      // 生成近半年数据（按月）
      interface MonthlyOrderStat {
        monthLabel: string
        clientName: string
        versionIterationCount: number
        deliveryCount: number
      }
      const monthlyData: MonthlyOrderStat[] = []
      for (let i = 5; i >= 0; i--) {
        const monthStart = new Date(today)
        monthStart.setMonth(monthStart.getMonth() - i)
        monthStart.setDate(1)

        const monthLabel = `${monthStart.getFullYear()}-${String(monthStart.getMonth() + 1).padStart(2, '0')}`

        clients.forEach((client) => {
          monthlyData.push({
            monthLabel,
            clientName: client,
            versionIterationCount: client === 'CAE' ? Math.floor(Math.random() * 25) + 15 : Math.floor(Math.random() * 35) + 25, // CAE: 15-39个, 华模: 25-59个
            deliveryCount: client === 'CAE' ? Math.floor(Math.random() * 40) + 20 : Math.floor(Math.random() * 50) + 35, // CAE: 20-59个, 华模: 35-84个
          })
        })
      }

      return {
        error: '',
        status: 1,
        data: {
          daily: dailyData,
          weekly: weeklyData,
          monthly: monthlyData,
        },
        message: 'success',
      }
    },
  },

])
