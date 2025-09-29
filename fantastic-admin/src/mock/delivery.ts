import { defineFakeRoute } from 'vite-plugin-fake-server/client'

const dongles = [
  {
    dongleId: 'DONGLE001',
    shellCode: 'A53000000001',
    shellSerial: '89172CAS240919239001890',
    dongleRemark: '正常使用中，无异常情况',
    dongleStatus: '出库',
    licenses: [
      {
        licenseId: 'AUTH-123456',
        type: '本地锁',
        startDate: '2024-01-01',
        endDate: '2024-12-31',
        licenseStatus: '授权中',
        licenseRemark: '标准授权',
      },
      {
        licenseId: 'AUTH-234567',
        type: '网络锁',
        startDate: '2024-02-01',
        endDate: '2024-11-30',
        licenseStatus: '授权中',
        licenseRemark: '网络授权',
      },
    ],
    clientName: '华模科技',
    deviceType: 'lab',
    note: '测试环境专用设备',
    inTime: '2024-01-15 09:00:00',
    outTime: '2024-02-01 14:30:00',
  },
  {
    dongleId: 'DONGLE002',
    shellCode: 'A53000000002',
    shellSerial: '89172CAS240919239001891',
    dongleRemark: '备用设备',
    dongleStatus: '入库',
    licenses: [],
    clientName: '',
    deviceType: '',
    note: '',
    inTime: '2024-01-20 10:15:00',
    outTime: '',
  },
  {
    dongleId: 'DONGLE003',
    shellCode: 'A53000000003',
    shellSerial: '89172CAS240919239001892',
    dongleRemark: '设备损坏，需要维修',
    dongleStatus: '损坏',
    licenses: [
      {
        licenseId: 'AUTH-345678',
        type: '软锁授权',
        startDate: '2024-01-01',
        endDate: '2024-06-30',
        licenseStatus: '已过期',
        licenseRemark: '已过期授权',
      },
    ],
    clientName: '测试客户',
    deviceType: 'IPT',
    note: '曾用于IPT测试',
    inTime: '2024-01-10 08:30:00',
    outTime: '2024-01-25 16:00:00',
  },
  {
    dongleId: 'DONGLE004',
    shellCode: 'A53000000004',
    shellSerial: '89172CAS240919239001893',
    dongleRemark: '设备丢失',
    dongleStatus: '丢失',
    licenses: [
      {
        licenseId: 'AUTH-456789',
        type: '本地锁',
        startDate: '2023-12-01',
        endDate: '2024-11-30',
        licenseStatus: '未授权',
        licenseRemark: '丢失前的授权',
      },
    ],
    clientName: '某客户',
    deviceType: 'FTD',
    note: '客户现场丢失',
    inTime: '2023-12-15 11:00:00',
    outTime: '2024-01-05 09:30:00',
  },
]

export default defineFakeRoute([
// 获取客户列表信息
  {
    url: '/mock/client/list',
    method: 'get',
    response: () => {
      return {
        error: '',
        status: 1,
        data: {
          list: [
            {
              clientID: 1, // 客户ID
              clientName: 'CAE',
              dongleCount: 6,
              modelCount: 3, // 3种不同模型：ATA04_Aerodynamics、ATA08_WeightBalance、ATA00_Groundhandling
              modelVersionCount: 16, // 16个模型版本组合（工单数量）
              licenseStats: {
                validCount: 8,
                expiringCount: 5,
                expiredCount: 3,
              },
            },
            {
              clientID: 2,
              clientName: '华模科技',
              dongleCount: 3,
              modelCount: 3, // 3种不同模型
              modelVersionCount: 12, // 12个模型版本组合（工单数量）
              licenseStats: {
                validCount: 4,
                expiringCount: 2,
                expiredCount: 1,
              },
            },
          ],
          total: 2,
        },
      }
    },
  },

  // 获取授权详情信息（概览模式）
  {
    url: '/mock/client/auth/detail',
    method: 'get',
    response: ({ query }) => {
      const { clientName } = query

      // 只为CAE和华模科技返回数据
      if (clientName === 'CAE') {
        return {
          error: '',
          status: 1,
          data: {
            clientName: 'CAE',
            dongleCount: 6,
            shellNumbers: [
              {
                shellNumber: 'A53000000001',
                deviceType: 'lab',
                deviceNote: '实验室环境用于研发测试的设备',
                authCount: 3, // 该外壳号下的总授权数量
                authorizationList: [
                  // 只返回第一条授权记录
                  {
                    authId: '2025071001',
                    startDate: '2025-01-01',
                    endDate: '2025-12-31',
                    authType: '本地锁',
                    authNote: 'CAE计算模块本地锁授权，适用于单机离线计算',
                    status: '有效',
                  },
                ],
              },
              {
                shellNumber: 'A53000000002',
                deviceType: 'IPT',
                deviceNote: '集成产品团队专用的协同开发平台',
                authCount: 2,
                authorizationList: [
                  {
                    authId: '2025071004',
                    startDate: '2025-03-01',
                    endDate: '2025-11-30',
                    authType: '网络锁',
                    authNote: '热传导分析网络锁，支持团队协作和资源共享',
                    status: '有效',
                  },
                ],
              },
              {
                shellNumber: 'A53000000003',
                deviceType: 'FTD',
                deviceNote: '飞行试验数据处理系统专用设备',
                authCount: 3,
                authorizationList: [
                  {
                    authId: '2025071006',
                    startDate: '2025-04-01',
                    endDate: '2025-10-31',
                    authType: '软锁授权',
                    authNote: '新功能模块软锁试用，用于飞行数据分析验证',
                    status: '有效',
                  },
                ],
              },
              {
                shellNumber: 'A53000000004',
                deviceType: 'FFS',
                deviceNote: '全功能仿真系统，用于复杂工程仿真计算',
                authCount: 2,
                authorizationList: [
                  {
                    authId: '2025071009',
                    startDate: '2025-06-01',
                    endDate: '2026-05-31',
                    authType: '网络锁',
                    authNote: '全功能仿真套件网络锁，包含所有分析模块',
                    status: '有效',
                  },
                ],
              },
              {
                shellNumber: 'A53000000005',
                deviceType: 'lab',
                deviceNote: '测试环境专用设备，仅有临期授权',
                authCount: 3,
                authorizationList: [
                  {
                    authId: '2025071011',
                    startDate: '2025-01-01',
                    endDate: '2025-08-02',
                    authType: '软锁授权',
                    authNote: '基础功能软锁授权，2天后到期需及时续期',
                    status: '临期',
                  },
                ],
              },
              {
                shellNumber: 'A53000000006',
                deviceType: 'FTD',
                deviceNote: '旧版飞行数据处理设备，所有授权已过期',
                authCount: 3,
                authorizationList: [
                  {
                    authId: '2025071014',
                    startDate: '2023-01-01',
                    endDate: '2024-01-01',
                    authType: '本地锁',
                    authNote: '已过期超过200天的本地锁授权，需要重新购买',
                    status: '过期',
                  },
                ],
              },
            ],
          },
        }
      }

      if (clientName === '华模科技') {
        return {
          error: '',
          status: 1,
          data: {
            clientName: '华模科技',
            dongleCount: 3,
            shellNumbers: [
              {
                shellNumber: 'A53000000007',
                deviceType: 'IPT',
                deviceNote: '华模科技集成产品团队专用设备',
                authCount: 2,
                authorizationList: [
                  {
                    authId: '2025071017',
                    startDate: '2025-01-01',
                    endDate: '2026-01-01',
                    authType: '本地锁',
                    authNote: '华模科技专用本地锁授权，长期有效',
                    status: '有效',
                  },
                ],
              },
              {
                shellNumber: 'A53000000008',
                deviceType: 'FFS',
                deviceNote: '华模科技全功能仿真系统',
                authCount: 2,
                authorizationList: [
                  {
                    authId: '2025071019',
                    startDate: '2025-02-01',
                    endDate: '2025-11-30',
                    authType: '网络锁',
                    authNote: '华模科技仿真系统网络锁授权',
                    status: '有效',
                  },
                ],
              },
              {
                shellNumber: 'A53000000009',
                deviceType: 'lab',
                deviceNote: '华模科技实验室测试设备',
                authCount: 3,
                authorizationList: [
                  {
                    authId: '2025071021',
                    startDate: '2025-04-01',
                    endDate: '2025-10-31',
                    authType: '本地锁',
                    authNote: '华模科技实验室本地锁授权',
                    status: '有效',
                  },
                ],
              },
            ],
          },
        }
      }

      // 其他客户返回空数据
      return {
        error: '',
        status: 1,
        data: {
          clientName: clientName || '未知客户',
          dongleCount: 0,
          totalAuthCount: 0,
          validAuthCount: 0,
          expiringAuthCount: 0,
          expiredAuthCount: 0,
          shellNumbers: [],
        },
      }
    },
  },

  // 新增：获取单个外壳号的完整授权列表
  {
    url: '/mock/shell/auth/list',
    method: 'get',
    response: ({ query }) => {
      const { clientName, shellNumber } = query

      // CAE客户的完整授权数据
      if (clientName === 'CAE') {
        const shellAuthData = {
          A53000000001: [
            {
              authId: '2025071001',
              startDate: '2025-01-01',
              endDate: '2025-12-31',
              authType: '本地锁',
              authNote: 'CAE计算模块本地锁授权，适用于单机离线计算',
              status: '有效',
            },
            {
              authId: '2025071002',
              startDate: '2025-02-01',
              endDate: '2025-08-10',
              authType: '软锁授权',
              authNote: '结构分析模块软锁试用版，基于软件序列号验证',
              status: '临期',
            },
            {
              authId: '2025071003',
              startDate: '2024-06-01',
              endDate: '2025-06-30',
              authType: '网络锁',
              authNote: '流体分析模块网络锁授权，支持多用户并发使用',
              status: '过期',
            },
          ],
          A53000000002: [
            {
              authId: '2025071004',
              startDate: '2025-03-01',
              endDate: '2025-11-30',
              authType: '网络锁',
              authNote: '热传导分析网络锁，支持团队协作和资源共享',
              status: '有效',
            },
            {
              authId: '2025071005',
              startDate: '2025-01-15',
              endDate: '2025-09-15',
              authType: '本地锁',
              authNote: '优化算法模块本地锁，提供高性能计算能力',
              status: '有效',
            },
          ],
          A53000000003: [
            {
              authId: '2025071006',
              startDate: '2025-04-01',
              endDate: '2025-10-31',
              authType: '软锁授权',
              authNote: '新功能模块软锁试用，用于飞行数据分析验证',
              status: '有效',
            },
            {
              authId: '2025071007',
              startDate: '2025-05-01',
              endDate: '2025-08-05',
              authType: '网络锁',
              authNote: '高级计算模块网络锁，支持分布式计算处理',
              status: '临期',
            },
            {
              authId: '2025071008',
              startDate: '2024-01-01',
              endDate: '2025-01-01',
              authType: '本地锁',
              authNote: '已过期的企业版本地锁授权，需要续期',
              status: '过期',
            },
          ],
          A53000000004: [
            {
              authId: '2025071009',
              startDate: '2025-06-01',
              endDate: '2026-05-31',
              authType: '网络锁',
              authNote: '全功能仿真套件网络锁，包含所有分析模块',
              status: '有效',
            },
            {
              authId: '2025071010',
              startDate: '2025-07-01',
              endDate: '2025-08-03',
              authType: '软锁授权',
              authNote: '专业插件软锁授权，即将到期请及时续费',
              status: '临期',
            },
          ],
          A53000000005: [
            {
              authId: '2025071011',
              startDate: '2025-01-01',
              endDate: '2025-08-02',
              authType: '软锁授权',
              authNote: '基础功能软锁授权，2天后到期需及时续期',
              status: '临期',
            },
            {
              authId: '2025071012',
              startDate: '2025-03-01',
              endDate: '2025-09-01',
              authType: '本地锁',
              authNote: '核心计算模块本地锁，明天到期请联系续费',
              status: '有效',
            },
            {
              authId: '2025071013',
              startDate: '2024-01-01',
              endDate: '2024-12-31',
              authType: '网络锁',
              authNote: '已过期的网络锁授权，所有功能已停用',
              status: '过期',
            },
          ],
          A53000000006: [
            {
              authId: '2025071014',
              startDate: '2023-01-01',
              endDate: '2024-01-01',
              authType: '本地锁',
              authNote: '已过期超过200天的本地锁授权，需要重新购买',
              status: '过期',
            },
            {
              authId: '2025071015',
              startDate: '2023-06-01',
              endDate: '2024-06-01',
              authType: '软锁授权',
              authNote: '已过期的软锁授权，功能已被禁用',
              status: '过期',
            },
            {
              authId: '2025071016',
              startDate: '2024-01-01',
              endDate: '2024-12-31',
              authType: '网络锁',
              authNote: '去年到期的网络锁，需要联系销售续费',
              status: '过期',
            },
          ],
        }

        const shellNum = Array.isArray(shellNumber) ? shellNumber[0] : shellNumber
        const authList = shellAuthData[shellNum as keyof typeof shellAuthData] || []

        return {
          error: '',
          status: 1,
          data: {
            shellNumber,
            authorizationList: authList,
          },
        }
      }

      // 华模科技客户的完整授权数据
      if (clientName === '华模科技') {
        const shellAuthData = {
          A53000000007: [
            {
              authId: '2025071017',
              startDate: '2025-01-01',
              endDate: '2026-01-01',
              authType: '本地锁',
              authNote: '华模科技专用本地锁授权，长期有效',
              status: '有效',
            },
            {
              authId: '2025071018',
              startDate: '2025-03-01',
              endDate: '2025-12-31',
              authType: '网络锁',
              authNote: '华模科技网络锁授权，支持团队协作',
              status: '有效',
            },
          ],
          A53000000008: [
            {
              authId: '2025071019',
              startDate: '2025-02-01',
              endDate: '2025-11-30',
              authType: '网络锁',
              authNote: '华模科技仿真系统网络锁授权',
              status: '有效',
            },
            {
              authId: '2025071020',
              startDate: '2025-06-01',
              endDate: '2025-08-05',
              authType: '软锁授权',
              authNote: '华模科技软锁试用版，即将到期',
              status: '临期',
            },
          ],
          A53000000009: [
            {
              authId: '2025071021',
              startDate: '2025-04-01',
              endDate: '2025-10-31',
              authType: '本地锁',
              authNote: '华模科技实验室本地锁授权',
              status: '有效',
            },
            {
              authId: '2025071022',
              startDate: '2025-07-01',
              endDate: '2025-08-02',
              authType: '软锁授权',
              authNote: '华模科技测试软锁，即将到期',
              status: '临期',
            },
            {
              authId: '2025071023',
              startDate: '2024-01-01',
              endDate: '2024-12-31',
              authType: '网络锁',
              authNote: '华模科技已过期的网络锁授权',
              status: '过期',
            },
          ],
        }

        const shellNum = Array.isArray(shellNumber) ? shellNumber[0] : shellNumber
        const authList = shellAuthData[shellNum as keyof typeof shellAuthData] || []

        return {
          error: '',
          status: 1,
          data: {
            shellNumber,
            authorizationList: authList,
          },
        }
      }

      // 其他客户返回空数据
      return {
        error: '',
        status: 1,
        data: {
          shellNumber,
          authorizationList: [],
        },
      }
    },
  },

  // 获取发送详情信息
  {
    url: '/mock/client/send/detail',
    method: 'get',
    response: ({ query }) => {
      const { clientName } = query

      // CAE技术有限公司的发送详情
      if (clientName === 'CAE') {
        return {
          error: '',
          status: 1,
          data: {
            clientName: 'CAE',
            modelCount: 3, // 3种不同的模型
            modelVersionCount: 16, // 16个模型版本组合（工单数量）
            dailySendRecords: [
              {
                date: '2025-07-31',
                sendCount: 5,
                records: [
                  {
                    modelId: 'ATA04_Aerodynamics',
                    modelVersionId: 'V3.2.1.2C',
                    orderId: 'WO202507310001',
                    sendTime: '2025-07-31 08:30:15',
                  },
                  {
                    modelId: 'ATA04_Aerodynamics',
                    modelVersionId: 'V3.2.3.1C',
                    orderId: 'WO202507310002',
                    sendTime: '2025-07-31 10:45:30',
                  },
                  {
                    modelId: 'ATA08_WeightBalance',
                    modelVersionId: 'V4.0.2.3A',
                    orderId: 'WO202507310003',
                    sendTime: '2025-07-31 14:20:45',
                  },
                  {
                    modelId: 'ATA00_Groundhandling',
                    modelVersionId: 'V1.9.8.1B',
                    orderId: 'WO202507310004',
                    sendTime: '2025-07-31 16:55:20',
                  },
                  {
                    modelId: 'ATA04_Aerodynamics',
                    modelVersionId: 'V3.1.0.2C',
                    orderId: 'WO202507310005',
                    sendTime: '2025-07-31 18:20:10',
                  },
                ],
              },
              {
                date: '2025-07-30',
                sendCount: 4,
                records: [
                  {
                    modelId: 'ATA04_Aerodynamics',
                    modelVersionId: 'V2.1.0.1C',
                    orderId: 'WO202507300001',
                    sendTime: '2025-07-30 09:15:30',
                  },
                  {
                    modelId: 'ATA08_WeightBalance',
                    modelVersionId: 'V1.5.2.2A',
                    orderId: 'WO202507300002',
                    sendTime: '2025-07-30 14:20:15',
                  },
                  {
                    modelId: 'ATA00_Groundhandling',
                    modelVersionId: 'V2.2.0.3B',
                    orderId: 'WO202507300003',
                    sendTime: '2025-07-30 16:45:20',
                  },
                  {
                    modelId: 'ATA08_WeightBalance',
                    modelVersionId: 'V4.1.0.1A',
                    orderId: 'WO202507300004',
                    sendTime: '2025-07-30 19:30:45',
                  },
                ],
              },
              {
                date: '2025-07-29',
                sendCount: 3,
                records: [
                  {
                    modelId: 'ATA04_Aerodynamics',
                    modelVersionId: 'V3.0.1.4C',
                    orderId: 'WO202507290001',
                    sendTime: '2025-07-29 10:30:45',
                  },
                  {
                    modelId: 'ATA08_WeightBalance',
                    modelVersionId: 'V1.8.0.2A',
                    orderId: 'WO202507290002',
                    sendTime: '2025-07-29 15:25:10',
                  },
                  {
                    modelId: 'ATA00_Groundhandling',
                    modelVersionId: 'V2.0.5.1B',
                    orderId: 'WO202507290003',
                    sendTime: '2025-07-29 17:40:20',
                  },
                ],
              },
              {
                date: '2025-07-28',
                sendCount: 2,
                records: [
                  {
                    modelId: 'ATA00_Groundhandling',
                    modelVersionId: 'V4.1.3.2B',
                    orderId: 'WO202507280001',
                    sendTime: '2025-07-28 11:45:30',
                  },
                  {
                    modelId: 'ATA04_Aerodynamics',
                    modelVersionId: 'V2.5.1.3C',
                    orderId: 'WO202507280002',
                    sendTime: '2025-07-28 14:15:45',
                  },
                ],
              },
              {
                date: '2025-07-27',
                sendCount: 2,
                records: [
                  {
                    modelId: 'ATA04_Aerodynamics',
                    modelVersionId: 'V2.3.4.1C',
                    orderId: 'WO202507270001',
                    sendTime: '2025-07-27 09:20:15',
                  },
                  {
                    modelId: 'ATA08_WeightBalance',
                    modelVersionId: 'V1.7.2.4A',
                    orderId: 'WO202507270002',
                    sendTime: '2025-07-27 13:40:50',
                  },
                ],
              },
            ],
          },
        }
      }

      // 华模科技的发送详情
      if (clientName === '华模科技') {
        return {
          error: '',
          status: 1,
          data: {
            clientName: '华模科技',
            modelCount: 3, // 3种不同的模型
            modelVersionCount: 12, // 12个模型版本组合（工单数量）
            dailySendRecords: [
              {
                date: '2025-07-31',
                sendCount: 3,
                records: [
                  {
                    modelId: 'ATA04_Aerodynamics',
                    modelVersionId: 'V2.8.1.2D',
                    orderId: 'WO202507310101',
                    sendTime: '2025-07-31 09:15:30',
                  },
                  {
                    modelId: 'ATA08_WeightBalance',
                    modelVersionId: 'V3.5.2.1B',
                    orderId: 'WO202507310102',
                    sendTime: '2025-07-31 13:45:15',
                  },
                  {
                    modelId: 'ATA00_Groundhandling',
                    modelVersionId: 'V1.6.3.3A',
                    orderId: 'WO202507310103',
                    sendTime: '2025-07-31 17:20:45',
                  },
                ],
              },
              {
                date: '2025-07-30',
                sendCount: 2,
                records: [
                  {
                    modelId: 'ATA04_Aerodynamics',
                    modelVersionId: 'V2.7.0.1D',
                    orderId: 'WO202507300101',
                    sendTime: '2025-07-30 10:30:20',
                  },
                  {
                    modelId: 'ATA08_WeightBalance',
                    modelVersionId: 'V3.4.1.2B',
                    orderId: 'WO202507300102',
                    sendTime: '2025-07-30 15:50:10',
                  },
                ],
              },
              {
                date: '2025-07-29',
                sendCount: 3,
                records: [
                  {
                    modelId: 'ATA04_Aerodynamics',
                    modelVersionId: 'V2.6.2.4D',
                    orderId: 'WO202507290101',
                    sendTime: '2025-07-29 08:45:15',
                  },
                  {
                    modelId: 'ATA08_WeightBalance',
                    modelVersionId: 'V3.3.0.1B',
                    orderId: 'WO202507290102',
                    sendTime: '2025-07-29 12:15:30',
                  },
                  {
                    modelId: 'ATA00_Groundhandling',
                    modelVersionId: 'V1.5.4.2A',
                    orderId: 'WO202507290103',
                    sendTime: '2025-07-29 16:40:20',
                  },
                ],
              },
              {
                date: '2025-07-28',
                sendCount: 2,
                records: [
                  {
                    modelId: 'ATA04_Aerodynamics',
                    modelVersionId: 'V2.5.3.1D',
                    orderId: 'WO202507280101',
                    sendTime: '2025-07-28 11:20:45',
                  },
                  {
                    modelId: 'ATA00_Groundhandling',
                    modelVersionId: 'V1.4.8.3A',
                    orderId: 'WO202507280102',
                    sendTime: '2025-07-28 14:35:10',
                  },
                ],
              },
              {
                date: '2025-07-26',
                sendCount: 2,
                records: [
                  {
                    modelId: 'ATA08_WeightBalance',
                    modelVersionId: 'V3.2.1.4B',
                    orderId: 'WO202507260101',
                    sendTime: '2025-07-26 09:50:15',
                  },
                  {
                    modelId: 'ATA00_Groundhandling',
                    modelVersionId: 'V1.3.9.2A',
                    orderId: 'WO202507260102',
                    sendTime: '2025-07-26 13:25:30',
                  },
                ],
              },
            ],
          },
        }
      }

      // 默认返回空数据
      return {
        error: '',
        status: 1,
        data: {
          clientName: clientName || '默认客户',
          modelCount: 0,
          modelVersionCount: 0,
          dailySendRecords: [],
        },
      }
    },
  },

  // 发送详情分页接口
  {
    url: '/mock/client/send/detail/pagination',
    method: 'get',
    response: ({ query }) => {
      const { clientName, page = 1, pageSize = 10 } = query

      // 参数验证
      if (!clientName) {
        return {
          error: '客户名称不能为空',
          status: 1,
          data: {
            success: false,
            message: '客户名称不能为空',
          },
        }
      }

      // 定义两个客户的完整发送数据
      const clientSendData = {
        CAE: {
          models: [
            'ATA04_Aerodynamics',
            'ATA08_WeightBalance',
            'ATA21_AirConditioning',
            'ATA27_FlightControls',
            'ATA32_LandingGear',
          ],
          // 生成15天的发送记录
          dailyRecords: generateCAEData(),
        },
        华模科技: {
          models: [
            'ATA04_Aerodynamics',
            'ATA12_Servicing',
            'ATA28_FuelSystem',
            'ATA36_PneumaticSystem',
          ],
          // 生成12天的发送记录
          dailyRecords: generateHuamoData(),
        },
      }

      // 生成CAE的发送数据
      function generateCAEData() {
        const records = []
        const baseDate = new Date('2024-08-06')

        // 生成15天的数据
        for (let i = 0; i < 15; i++) {
          const date = new Date(baseDate)
          date.setDate(baseDate.getDate() - i)
          const dateStr = date.toISOString().split('T')[0]

          interface DayRecord {
            modelId: string
            modelVersionId: string
            orderId: string
            sendTime: string
          }
          const dayRecords: DayRecord[] = []

          // 每天发送1-3个模型
          const modelsToday = ['ATA04_Aerodynamics', 'ATA08_WeightBalance', 'ATA21_AirConditioning'].slice(0, Math.floor(Math.random() * 3) + 1)

          modelsToday.forEach((modelId, index) => {
            const versionNum = Math.max(1, 5 - Math.floor(i / 3)) // 版本递减
            const platform = String.fromCharCode(67 - Math.floor(i / 5)) // C, B, A
            const modelVersionId = `V2.0.${versionNum}.1${platform}`

            const hour = 9 + index * 2
            const sendTime = `${dateStr} ${hour.toString().padStart(2, '0')}:${(index * 15).toString().padStart(2, '0')}:00`
            const orderId = Math.floor(Math.random() * 101).toString()

            dayRecords.push({
              modelId,
              modelVersionId,
              orderId,
              sendTime,
            })
          })

          if (dayRecords.length > 0) {
            records.push({
              date: dateStr,
              sendCount: dayRecords.length,
              records: dayRecords,
            })
          }
        }

        return records
      }

      // 生成华模科技的发送数据
      function generateHuamoData() {
        const records = []
        const baseDate = new Date('2024-08-05')

        // 生成12天的数据
        for (let i = 0; i < 12; i++) {
          const date = new Date(baseDate)
          date.setDate(baseDate.getDate() - i)
          const dateStr = date.toISOString().split('T')[0]

          interface DayRecord {
            modelId: string
            modelVersionId: string
            orderId: string
            sendTime: string
          }
          const dayRecords: DayRecord[] = []

          // 每天发送1-2个模型
          const modelsToday = ['ATA04_Aerodynamics', 'ATA12_Servicing'].slice(0, Math.floor(Math.random() * 2) + 1)

          modelsToday.forEach((modelId, index) => {
            const versionNum = Math.max(1, 4 - Math.floor(i / 4)) // 版本递减
            const platform = String.fromCharCode(66 - Math.floor(i / 6)) // B, A
            const modelVersionId = `V1.5.${versionNum}.2${platform}`

            const hour = 14 + index * 2
            const sendTime = `${dateStr} ${hour.toString().padStart(2, '0')}:${(index * 20).toString().padStart(2, '0')}:00`
            const orderId = Math.floor(Math.random() * 101).toString()

            dayRecords.push({
              modelId,
              modelVersionId,
              orderId,
              sendTime,
            })
          })

          if (dayRecords.length > 0) {
            records.push({
              date: dateStr,
              sendCount: dayRecords.length,
              records: dayRecords,
            })
          }
        }

        return records
      }

      const clientData = clientSendData[clientName as keyof typeof clientSendData]
      const allDailySendRecords = clientData.dailyRecords

      // 分页逻辑
      const startIndex = (Number(page) - 1) * Number(pageSize)
      const endIndex = startIndex + Number(pageSize)
      const paginatedRecords = allDailySendRecords.slice(startIndex, endIndex)

      return {
        error: '',
        status: 1,
        data: {
          clientName,
          page: Number(page),
          pageSize: Number(pageSize),
          total: allDailySendRecords.length, // 总日期数量
          dailySendRecords: paginatedRecords, // 当前页的日期记录
        },
      }
    },
  },

  // 获取发送总览
  {
    url: '/mock/client/send/overview',
    method: 'get',
    response: ({ query }) => {
      const { clientName } = query

      // 参数验证
      if (!clientName) {
        return {
          error: '客户名称不能为空',
          status: 1,
          data: {
            success: false,
            message: '客户名称不能为空',
          },
        }
      }

      // 定义各客户模型的最新版本（与分页数据保持一致）
      const clientLatestVersions = {
        CAE: [
          {
            modelId: 'ATA04_Aerodynamics',
            modelVersionId: 'V2.0.5.1C', // 最新版本
            orderId: '80',
            sendTime: '2024-08-06 09:00:00',
          },
          {
            modelId: 'ATA08_WeightBalance',
            modelVersionId: 'V2.0.5.1C',
            orderId: '81',
            sendTime: '2024-08-06 11:00:00',
          },
          {
            modelId: 'ATA21_AirConditioning',
            modelVersionId: 'V2.0.5.1C',
            orderId: '82',
            sendTime: '2024-08-06 13:00:00',
          },
          {
            modelId: 'ATA27_FlightControls',
            modelVersionId: 'V2.0.4.1C',
            orderId: '83',
            sendTime: '2024-08-03 10:00:00',
          },
          {
            modelId: 'ATA32_LandingGear',
            modelVersionId: 'V2.0.3.1B',
            orderId: '84',
            sendTime: '2024-08-01 15:00:00',
          },
        ],
        华模科技: [
          {
            modelId: 'ATA04_Aerodynamics',
            modelVersionId: 'V1.5.4.2B', // 最新版本
            orderId: 'HM-240805-001',
            sendTime: '2024-08-05 14:00:00',
          },
          {
            modelId: 'ATA12_Servicing',
            modelVersionId: 'V1.5.4.2B',
            orderId: 'HM-240805-002',
            sendTime: '2024-08-05 16:00:00',
          },
          {
            modelId: 'ATA28_FuelSystem',
            modelVersionId: 'V1.5.3.2B',
            orderId: 'HM-240802-001',
            sendTime: '2024-08-02 14:00:00',
          },
          {
            modelId: 'ATA36_PneumaticSystem',
            modelVersionId: 'V1.5.2.2A',
            orderId: 'HM-240730-001',
            sendTime: '2024-07-30 15:00:00',
          },
        ],
      }

      const latestVersionRecords = clientLatestVersions[clientName as keyof typeof clientLatestVersions] || []

      return {
        error: '',
        status: 1,
        data: {
          success: true,
          message: '获取发送总览成功',
          latestVersionRecords, // 各模型最新版本记录
        },
      }
    },
  },

  // 获取加密狗列表信息
  {
    url: '/mock/dongle/list',
    method: 'get',
    response: () => {
      return {
        error: '',
        status: 1,
        data: {
          list: [
            {
              dongleId: '1',
              shellCode: 'A53000000001', // 修改为新格式
              shellSerial: '89172CAS240919239001890',
              dongleRemark: 'CAE技术有限公司专用加密狗',
              dongleStatus: '出库',
              clientName: 'CAE',
              deviceType: 'lab',
              note: '实验室环境专用设备，用于CAE计算测试',
              inTime: '2024-05-06 10:00:00',
              outTime: '2024-07-15 14:30:00',
            },
            {
              dongleId: '2',
              shellCode: 'A53000000002', // 修改为新格式
              shellSerial: '89172CAS240919239001891',
              dongleRemark: '华模科技专用加密狗',
              dongleStatus: '出库',
              clientName: '华模科技',
              deviceType: 'IPT',
              note: '集成产品团队专用设备，支持多用户协作',
              inTime: '2024-07-20 09:00:00',
              outTime: '2024-07-25 16:00:00',
            },
            {
              dongleId: '3',
              shellCode: 'A53000000003', // 修改为新格式
              shellSerial: '89172CAS240919239001892',
              dongleRemark: '测试环境专用加密狗',
              dongleStatus: '入库',
              clientName: 'CAE',
              deviceType: 'FTD',
              note: '飞行测试数据处理系统专用设备',
              inTime: '2024-07-25 15:00:00',
              outTime: '',
            },
            {
              dongleId: '4',
              shellCode: 'A53000000004', // 修改为新格式
              shellSerial: '89172CAS240919239001893',
              dongleRemark: '新入库加密狗，待分配',
              dongleStatus: '入库',
              clientName: '暂无',
              deviceType: 'lab',
              note: '待分配给客户的新设备',
              inTime: '2025-07-28 10:00:00',
              outTime: '',
            },
            {
              dongleId: '5',
              shellCode: 'A53000000005', // 修改为新格式
              shellSerial: '89172CAS240919239001894',
              dongleRemark: '损坏设备，需要维修',
              dongleStatus: '损坏',
              clientName: '华模科技',
              deviceType: 'FFS',
              note: '全功能仿真系统，硬件故障待维修',
              inTime: '2024-01-15 14:00:00',
              outTime: '2024-06-20 10:30:00',
            },
            {
              dongleId: '6',
              shellCode: 'A53000000006', // 修改为新格式
              shellSerial: '89172CAS240919239001895',
              dongleRemark: '丢失设备记录',
              dongleStatus: '丢失',
              clientName: 'CAE',
              deviceType: 'lab',
              note: '实验室设备，已确认丢失',
              inTime: '2024-03-01 08:00:00',
              outTime: '2024-04-15 17:00:00',
            },
            {
              dongleId: '7',
              shellCode: 'A53000000007',
              shellSerial: '89172CAS240919239001896',
              dongleRemark: '备用加密狗',
              dongleStatus: '入库',
              clientName: '暂无',
              deviceType: 'lab',
              note: '备用设备，待分配',
              inTime: '2025-07-30 11:00:00',
              outTime: '',
            },
            {
              dongleId: '8',
              shellCode: 'A53000000008',
              shellSerial: '89172CAS240919239001897',
              dongleRemark: '高级授权加密狗',
              dongleStatus: '出库',
              clientName: 'CAE',
              deviceType: 'IPT',
              note: '高级功能测试设备',
              inTime: '2025-07-01 09:00:00',
              outTime: '2025-07-01 15:00:00',
            },
          ],
          total: 8,
        },
      }
    },
  },

  // 新增：获取加密狗历史记录
  {
    url: '/mock/dongle/:dongleId/history',
    method: 'get',
    response: ({ params }) => {
      let { dongleId } = params
      if (Array.isArray(dongleId)) {
        dongleId = dongleId[0]
      }

      // 根据dongleId返回新格式的历史记录
      const historyDataMap: Record<string, any[]> = {
        1: [
          {
            id: '1001',
            outTime: '2024-06-15 14:30:00', // 出库时间
            inTime: '2024-07-25 16:00:00', // 入库时间
            clientName: 'CAE技术有限公司',
            authorizations: [
              {
                generateDate: '2024-06-16',
                authId: '2025071001',
                startDate: '2024-06-15',
                endDate: '2024-12-31',
                authType: '本地锁',
                authNote: 'CAE计算模块本地锁授权，适用于单机离线计算',
              },
              {
                generateDate: '2024-07-01',
                authId: '2025071002',
                startDate: '2024-07-01',
                endDate: '2025-08-10',
                authType: '网络锁',
                authNote: '网络锁临时授权，即将到期',
              },
              {
                generateDate: '2024-07-15',
                authId: '2025071003',
                startDate: '2024-07-15',
                endDate: '2025-07-20',
                authType: '软锁授权',
                authNote: '软锁试用版授权，已到期',
              },
            ],
          },
          {
            id: '1002',
            outTime: '2024-05-06 10:00:00', // 出库时间
            inTime: '2024-06-15 14:30:00', // 入库时间
            clientName: 'CAE技术有限公司',
            authorizations: [
              {
                generateDate: '2024-05-10',
                authId: '2025071004',
                startDate: '2024-05-10',
                endDate: '2024-11-30',
                authType: '本地锁',
                authNote: '初期测试本地锁授权',
              },
            ],
          },
          {
            id: '1003',
            outTime: '未出库', // 出库时间
            inTime: '2024-05-06 10:00:00', // 入库时间
            clientName: '',
            authorizations: [], // 入库期间无授权
          },
        ],
        2: [
          {
            id: '2001',
            outTime: '2024-07-25 16:00:00', // 出库时间
            inTime: '至今', // 入库时间
            clientName: '华模科技',
            authorizations: [
              {
                generateDate: '2024-07-26',
                authId: '2025071005',
                startDate: '2024-07-25',
                endDate: '2026-08-30',
                authType: '本地锁',
                authNote: '华模科技专用本地锁授权',
              },
              {
                generateDate: '2024-08-01',
                authId: '2025071006',
                startDate: '2024-08-01',
                endDate: '2025-12-31',
                authType: '网络锁',
                authNote: '华模科技网络锁扩展授权',
              },
            ],
          },
          {
            id: '2002',
            outTime: '未出库', // 出库时间
            inTime: '2024-07-25 16:00:00', // 入库时间
            clientName: '',
            authorizations: [], // 短期入库无授权
          },
        ],
        3: [
          {
            id: '3001',
            outTime: '2024-07-25 15:00:00', // 出库时间
            inTime: '至今', // 入库时间
            clientName: 'CAE技术有限公司',
            authorizations: [
              {
                generateDate: '2024-07-26',
                authId: '2025071007',
                startDate: '2024-07-25',
                endDate: '2025-08-03',
                authType: '网络锁',
                authNote: '测试环境网络锁，即将过期需续费',
              },
            ],
          },
        ],
        4: [
          {
            id: '4001',
            outTime: '2025-07-28 10:00:00', // 出库时间
            inTime: '未入库', // 入库时间
            clientName: '暂无',
            authorizations: [],
          },
        ],
        5: [
          {
            id: '5001',
            outTime: '2024-06-20 10:30:00', // 出库时间
            inTime: '2024-07-15 14:00:00', // 入库时间（损坏后入库）
            clientName: '华模科技',
            authorizations: [
              {
                generateDate: '2024-01-16',
                authId: '2025071007',
                startDate: '2024-01-01',
                endDate: '2024-12-31',
                authType: '本地锁',
                authNote: '设备损坏前的授权，已过期',
              },
            ],
          },
          {
            id: '5002',
            outTime: '未出库', // 出库时间
            inTime: '2024-01-15 14:00:00', // 入库时间
            clientName: '',
            authorizations: [],
          },
        ],
        6: [
          {
            id: '6001',
            outTime: '2024-04-15 17:00:00', // 出库时间
            inTime: '丢失', // 入库时间（丢失）
            clientName: 'CAE',
            authorizations: [
              {
                generateDate: '2024-03-02',
                authId: '2025071008',
                startDate: '2024-03-01',
                endDate: '2025-03-01',
                authType: '软锁授权',
                authNote: '丢失设备的软锁授权，已停用',
              },
            ],
          },
          {
            id: '6002',
            outTime: '未出库', // 出库时间
            inTime: '2024-03-01 08:00:00', // 入库时间
            clientName: '',
            authorizations: [],
          },
        ],
        7: [
          {
            id: '7001',
            outTime: '未出库', // 出库时间
            inTime: '2025-07-30 11:00:00', // 入库时间
            clientName: '暂无',
            authorizations: [
              {
                generateDate: '2025-01-02',
                authId: '2025071009',
                startDate: '2025-01-01',
                endDate: '2025-12-31',
                authType: '本地锁',
                authNote: '备用设备本地锁授权',
              },
            ],
          },
        ],
        8: [
          {
            id: '8001',
            outTime: '2025-07-01 15:00:00', // 出库时间
            inTime: '至今', // 入库时间
            clientName: 'CAE',
            authorizations: [
              {
                generateDate: '2025-07-02',
                authId: '2025071010',
                startDate: '2025-07-01',
                endDate: '2026-07-01',
                authType: '网络锁',
                authNote: '高级功能网络锁授权，一年期',
              },
              {
                generateDate: '2025-07-11',
                authId: '2025071011',
                startDate: '2025-07-10',
                endDate: '2025-12-31',
                authType: '软锁授权',
                authNote: '扩展模块软锁授权',
              },
            ],
          },
          {
            id: '8002',
            outTime: '未出库', // 出库时间
            inTime: '2025-07-01 09:00:00', // 入库时间
            clientName: '',
            authorizations: [],
          },
        ],
      }

      // 获取对应dongleId的历史记录
      const historyList = historyDataMap[dongleId] || []

      return {
        error: '',
        status: 1,
        data: {
          dongleId,
          total: historyList.length,
          list: historyList,
        },
      }
    },
  },

  // 创建加密狗接口
  {
    url: '/mock/dongle/create',
    method: 'post',
    response: ({ body }) => {
      const { shellCode, shellSerial } = body

      // 基本验证
      if (!shellCode || !shellSerial) {
        return {
          error: '参数不完整',
          status: 1,
          data: {
            success: false,
            message: '外壳号和序列号不能为空',
          },
        }
      }

      // 模拟检查重复外壳号（可选）
      const existingDongles = [
        'A53000000001',
        'A53000000002',
        'A53000000003',
        'A53000000004',
        'A53000000005',
        'A53000000006',
      ]

      if (existingDongles.includes(shellCode)) {
        return {
          error: '外壳号已存在',
          status: 1,
          data: {
            success: false,
            message: '该外壳号已存在，请使用其他外壳号',
          },
        }
      }

      // 模拟成功创建
      return {
        error: '',
        status: 1,
        data: {
          success: true,
          message: '创建加密狗成功',
        },
      }
    },
  },

  // 更新加密狗信息
  {
    url: '/mock/delivery/dongles/update',
    method: 'put',
    response: ({ body }: any) => {
      const { dongleId, shellCode, shellSerial } = body

      const dongleIndex = dongles.findIndex(d => d.dongleId === dongleId)
      if (dongleIndex !== -1) {
        dongles[dongleIndex].shellCode = shellCode
        dongles[dongleIndex].shellSerial = shellSerial

        return {
          error: '',
          status: 1,
          data: {
            success: true,
            message: '更新加密狗信息成功',
          },
        }
      }

      return {
        error: '加密狗不存在',
        status: 1,
        data: {
          success: false,
          message: '加密狗不存在',
        },
      }
    },
  },

  // 新建客户
  {
    url: '/mock/client/create',
    method: 'post',
    response: ({ body }) => {
      const { clientName, clientinfo } = body

      // 模拟数据验证
      if (!clientName) {
        return {
          error: '客户名称不能为空',
          status: 0,
          data: {
            success: false,
            message: '客户名称是必填项，请输入客户名称',
          },
        }
      }

      if (clientName.length < 2 || clientName.length > 50) {
        return {
          error: '客户名称长度不符合要求',
          status: 0,
          data: {
            success: false,
            message: '客户名称长度必须在2到50个字符之间',
          },
        }
      }

      if (clientinfo && clientinfo.length > 200) {
        return {
          error: '客户信息备注过长',
          status: 0,
          data: {
            success: false,
            message: '客户信息备注不能超过200个字符',
          },
        }
      }

      // 模拟检查客户名称是否已存在
      const existingClients = ['CAE技术有限公司', '华模科技']
      if (existingClients.includes(clientName)) {
        return {
          error: '客户名称已存在',
          status: 0,
          data: {
            success: false,
            message: `客户名称 "${clientName}" 已存在，请使用其他名称`,
          },
        }
      }

      // 模拟创建成功 - 只返回成功状态
      return {
        error: '',
        status: 1,
        data: {
          success: true,
          message: `客户 "${clientName}" 创建成功`,
        },
      }
    },
  },

  // 编辑客户
  {
    url: '/mock/client/update',
    method: 'put',
    response: ({ body }) => {
      const { originalClientName, clientName, clientinfo } = body

      // 模拟数据验证
      if (!originalClientName) {
        return {
          error: '原始客户名称不能为空',
          status: 0,
          data: {
            success: false,
            message: '原始客户名称参数缺失',
          },
        }
      }

      if (!clientName) {
        return {
          error: '客户名称不能为空',
          status: 0,
          data: {
            success: false,
            message: '客户名称是必填项，请输入客户名称',
          },
        }
      }

      if (clientName.length < 2 || clientName.length > 50) {
        return {
          error: '客户名称长度不符合要求',
          status: 0,
          data: {
            success: false,
            message: '客户名称长度必须在2到50个字符之间',
          },
        }
      }

      if (clientinfo && clientinfo.length > 200) {
        return {
          error: '客户信息备注过长',
          status: 0,
          data: {
            success: false,
            message: '客户信息备注不能超过200个字符',
          },
        }
      }

      // 模拟检查原始客户是否存在
      const existingClients = ['CAE', '华模科技']
      if (!existingClients.includes(originalClientName)) {
        return {
          error: '客户不存在',
          status: 0,
          data: {
            success: false,
            message: `客户 "${originalClientName}" 不存在，无法修改`,
          },
        }
      }

      // 如果修改了客户名称，检查新名称是否已存在
      if (originalClientName !== clientName) {
        if (existingClients.includes(clientName)) {
          return {
            error: '客户名称已存在',
            status: 0,
            data: {
              success: false,
              message: `客户名称 "${clientName}" 已存在，请使用其他名称`,
            },
          }
        }
      }

      // 模拟编辑成功
      return {
        error: '',
        status: 1,
        data: {
          success: true,
          message: originalClientName === clientName
            ? `客户 "${clientName}" 信息修改成功`
            : `客户名称已从 "${originalClientName}" 修改为 "${clientName}"`,
        },
      }
    },
  },

  // 交付外壳
  {
    url: '/mock/shell/deliver',
    method: 'post',
    response: ({ body }) => {
      const { clientName, shellNumber, deviceType, deviceNote } = body

      // 基本参数验证
      if (!clientName || !shellNumber || !deviceType || !deviceNote) {
        return {
          error: '参数不完整',
          status: 1,
          data: {
            success: false,
            message: '客户名称、外壳号、设备类型、设备备注都不能为空',
          },
        }
      }

      // 客户名称验证
      if (clientName.length < 2 || clientName.length > 50) {
        return {
          error: '客户名称长度不符合要求',
          status: 1,
          data: {
            success: false,
            message: '客户名称长度必须在2到50个字符之间',
          },
        }
      }

      // 外壳号格式验证
      const shellNumberPattern = /^A\d{11}$/
      if (!shellNumberPattern.test(shellNumber)) {
        return {
          error: '外壳号格式错误',
          status: 1,
          data: {
            success: false,
            message: '外壳号格式不正确，应为A开头+11位数字，如：A53000000001',
          },
        }
      }

      // 设备类型验证
      const validDeviceTypes = ['lab', 'IPT', 'FTD', 'FFS']
      if (!validDeviceTypes.includes(deviceType)) {
        return {
          error: '设备类型无效',
          status: 1,
          data: {
            success: false,
            message: '设备类型必须是：lab、IPT、FTD、FFS 中的一种',
          },
        }
      }

      // 设备备注长度验证
      if (deviceNote.length < 1 || deviceNote.length > 200) {
        return {
          error: '设备备注长度不符合要求',
          status: 1,
          data: {
            success: false,
            message: '设备备注长度必须在1到200个字符之间',
          },
        }
      }

      // 模拟检查客户是否存在
      const existingClients = ['CAE', '华模科技']
      if (!existingClients.includes(clientName)) {
        return {
          error: '客户不存在',
          status: 1,
          data: {
            success: false,
            message: `客户 "${clientName}" 不存在，请先创建客户信息`,
          },
        }
      }

      // 模拟检查外壳号是否已被使用
      const usedShellNumbers = [
        'A53000000001',
        'A53000000002',
        'A53000000003',
        'A53000000004',
        'A53000000005',
        'A53000000006',
        'A53000000007',
        'A53000000008',
        'A53000000009',
      ]
      if (usedShellNumbers.includes(shellNumber)) {
        return {
          error: '外壳号已被使用',
          status: 1,
          data: {
            success: false,
            message: `外壳号 "${shellNumber}" 已被使用，请使用其他外壳号`,
          },
        }
      }

      // 模拟检查外壳号是否存在于库存中
      const availableShellNumbers = [
        'A53000000010',
        'A53000000011',
        'A53000000012',
        'A53000000013',
        'A53000000014',
        'A53000000015',
      ]
      if (!availableShellNumbers.includes(shellNumber)) {
        return {
          error: '外壳号不存在',
          status: 1,
          data: {
            success: false,
            message: `外壳号 "${shellNumber}" 不存在于库存中，请检查外壳号是否正确`,
          },
        }
      }

      // 模拟不同的成功场景
      const successMessages = [
        `外壳号 ${shellNumber} 已成功交付给客户 "${clientName}"`,
        `交付完成！外壳号 ${shellNumber} 现已分配给客户 "${clientName}"`,
        `客户 "${clientName}" 的设备 ${shellNumber} 交付成功，设备类型：${deviceType}`,
      ]

      const randomMessage = successMessages[Math.floor(Math.random() * successMessages.length)]

      // 模拟交付成功
      return {
        error: '',
        status: 1,
        data: {
          success: true,
          message: randomMessage,
          details: {
            clientName,
            shellNumber,
            deviceType,
            deviceNote,
            deliveryTime: new Date().toISOString().replace('T', ' ').substring(0, 19), // 交付时间
            deliveryId: `DLV${Date.now()}`, // 交付单号
          },
        },
      }
    },
  },

  // 归还外壳
  {
    url: '/mock/shell/return',
    method: 'post',
    response: ({ body }) => {
      const { clientName, shellNumber, returnDate } = body

      // 基本参数验证
      if (!clientName || !shellNumber || !returnDate) {
        return {
          error: '参数不完整',
          status: 0,
          data: {
            success: false,
            message: '客户名称、外壳号、入库时间、归还备注都不能为空',
          },
        }
      }

      // 客户名称验证
      if (clientName.length < 2 || clientName.length > 50) {
        return {
          error: '客户名称长度不符合要求',
          status: 0,
          data: {
            success: false,
            message: '客户名称长度必须在2到50个字符之间',
          },
        }
      }

      // 外壳号格式验证
      const shellNumberPattern = /^A\d{11}$/
      if (!shellNumberPattern.test(shellNumber)) {
        return {
          error: '外壳号格式错误',
          status: 0,
          data: {
            success: false,
            message: '外壳号格式不正确，应为A开头+11位数字，如：A53000000001',
          },
        }
      }

      // 日期格式验证
      const datePattern = /^\d{4}-\d{2}-\d{2}$/
      if (!datePattern.test(returnDate)) {
        return {
          error: '日期格式错误',
          status: 0,
          data: {
            success: false,
            message: '入库时间格式不正确，应为YYYY-MM-DD格式',
          },
        }
      }

      // 日期范围验证
      const returnDateObj = new Date(returnDate)
      const today = new Date()
      today.setHours(23, 59, 59, 999) // 设置为今天结束时间

      if (returnDateObj > today) {
        return {
          error: '日期超出范围',
          status: 0,
          data: {
            success: false,
            message: '入库时间不能选择未来日期',
          },
        }
      }

      // 模拟检查客户是否存在
      const existingClients = ['CAE', '华模科技', '中科院']
      if (!existingClients.includes(clientName)) {
        return {
          error: '客户不存在',
          status: 0,
          data: {
            success: false,
            message: `客户 "${clientName}" 不存在，请检查客户名称`,
          },
        }
      }

      // 模拟检查外壳号是否属于该客户
      const clientShellMapping = {
        CAE: ['A53000000001', 'A53000000002', 'A53000000003'],
        华模科技: ['A53000000004', 'A53000000005'],
        中科院: ['A53000000006', 'A53000000007', 'A53000000008'],
      }

      const clientShells = clientShellMapping[clientName as keyof typeof clientShellMapping] || []
      if (!clientShells.includes(shellNumber)) {
        return {
          error: '外壳号不属于该客户',
          status: 0,
          data: {
            success: false,
            message: `外壳号 "${shellNumber}" 不属于客户 "${clientName}"，无法归还`,
          },
        }
      }

      // 模拟检查外壳号是否已归还
      const returnedShells = ['A53000000009', 'A53000000010']
      if (returnedShells.includes(shellNumber)) {
        return {
          error: '外壳号已归还',
          status: 0,
          data: {
            success: false,
            message: `外壳号 "${shellNumber}" 已归还，无法重复归还`,
          },
        }
      }

      // 模拟不同的成功场景
      const successMessages = [
        `外壳号 ${shellNumber} 已成功归还，入库时间：${returnDate}`,
        `归还完成！外壳号 ${shellNumber} 已回收到库存，客户：${clientName}`,
        `客户 "${clientName}" 的设备 ${shellNumber} 归还成功}`,
      ]

      const randomMessage = successMessages[Math.floor(Math.random() * successMessages.length)]

      // 模拟归还成功
      return {
        error: '',
        status: 1,
        data: {
          success: true,
          message: randomMessage,
        },
      }
    },
  },

  // 更新外壳号信息
  {
    url: '/mock/shell/update',
    method: 'put',
    response: ({ body }) => {
      const { clientName, shellNumber, deviceType } = body

      // 模拟验证
      if (!clientName || !shellNumber) {
        return {
          error: '客户名称和外壳号不能为空',
          status: 1,
          data: {
            success: false,
            message: '参数错误：客户名称和外壳号不能为空',
          },
        }
      }

      if (!['lab', 'IPT', 'FTD', 'FFS'].includes(deviceType)) {
        return {
          error: '设备类型无效',
          status: 1,
          data: {
            success: false,
            message: '设备类型必须是：lab、IPT、FTD、FFS 中的一种',
          },
        }
      }

      return {
        error: '',
        status: 1,
        data: {
          success: true,
          message: `外壳号 ${shellNumber} 信息更新成功`,
        },
      }
    },
  },

  // 新建授权信息
  {
    url: '/mock/auth/create',
    method: 'post',
    response: ({ body }) => {
      const { clientName, shellNumber, authId, authType, startDate, endDate } = body

      // 模拟验证
      if (!clientName || !shellNumber || !authId || !authType || !startDate || !endDate) {
        return {
          error: '必填参数缺失',
          status: 0,
          data: {
            success: false,
            message: '客户名称、外壳号、授权ID、授权类型、开始日期、结束日期都不能为空',
          },
        }
      }

      if (!['本地锁', '网络锁', '软锁授权'].includes(authType)) {
        return {
          error: '授权类型无效',
          status: 0,
          data: {
            success: false,
            message: '授权类型必须是：本地锁、网络锁、软锁授权 中的一种',
          },
        }
      }

      // 日期验证
      if (new Date(startDate) >= new Date(endDate)) {
        return {
          error: '日期无效',
          status: 0,
          data: {
            success: false,
            message: '授权结束日期必须大于开始日期',
          },
        }
      }

      return {
        error: '',
        status: 1,
        data: {
          success: true,
          message: `外壳号 ${shellNumber} 的授权 ${authId} 创建成功`,
        },
      }
    },
  },

  // 批量更新授权截止日期
  {
    url: '/mock/auth/batch-update',
    method: 'put',
    response: ({ body }) => {
      const { clientName, changes } = body

      // 模拟验证
      if (!clientName) {
        return {
          error: '客户名称不能为空',
          status: 0,
          data: {
            success: false,
            message: '客户名称不能为空',
          },
        }
      }

      if (!Array.isArray(changes) || changes.length === 0) {
        return {
          error: '没有变更数据',
          status: 0,
          data: {
            success: false,
            message: '没有检测到需要更新的授权数据',
          },
        }
      }

      // 验证每个变更项
      for (const change of changes) {
        if (!change.shellNumber || !change.authId || !change.newEndDate) {
          return {
            error: '变更数据不完整',
            status: 0,
            data: {
              success: false,
              message: '每个变更项都必须包含外壳号、授权ID和新的结束日期',
            },
          }
        }
      }

      // 模拟处理成功
      const successCount = changes.length
      const failedItems: string[] = []

      return {
        error: '',
        status: 1,
        data: {
          success: true,
          message: `批量更新完成，成功更新 ${successCount} 个授权${failedItems.length > 0 ? `，失败 ${failedItems.length} 个` : ''}`,
        },
      }
    },
  },

  // 新增：获取可交付外壳号列表
  {
    url: '/mock/delivery/available-shells',
    method: 'get',
    response: () => {
      // 模拟可交付的外壳号库存
      const availableShells = [
        'A53000000001',
        'A53000000002',
        'A53000000003',
        'A53000000004',
        'A53000000005',
        'A53000000006',
        'A53000000007',
        'A53000000008',
      ]

      return {
        error: '',
        status: 1,
        data: {
          shells: availableShells,
          total: availableShells.length,
        },
        message: 'success',
      }
    },
  },

  // 获取客户模型版本历史信息
  {
    url: '/mock/client/model-version-history',
    method: 'get',
    response: ({ query }) => {
      const { clientName } = query

      // 模拟不同客户的模型版本历史数据
      const mockData = {
        'CAE': {
          models: [
            {
              ata_code: 'ATA04',
              model_name: 'Aerodynamics',
              latest_version: 'v3.2',
              version_info: {
                '1': { version_id: 'VER001', version: 'v1.0', is_latest: false },
                '2': { version_id: 'VER002', version: 'v2.0', is_latest: false },
                '3': { version_id: 'VER003', version: 'v3.0', is_latest: false },
                '4': { version_id: 'VER004', version: 'v3.2', is_latest: true }
              }
            },
            {
              ata_code: 'ATA08',
              model_name: 'WeightBalance',
              latest_version: 'v2.5',
              version_info: {
                '1': { version_id: 'VER005', version: 'v1.0', is_latest: false },
                '2': { version_id: 'VER006', version: 'v2.0', is_latest: false },
                '3': { version_id: 'VER007', version: 'v2.5', is_latest: true }
              }
            },
            {
              ata_code: 'ATA00',
              model_name: 'Groundhandling',
              latest_version: 'v1.8',
              version_info: {
                '1': { version_id: 'VER008', version: 'v1.0', is_latest: false },
                '2': { version_id: 'VER009', version: 'v1.5', is_latest: false },
                '3': { version_id: 'VER010', version: 'v1.8', is_latest: true }
              }
            }
          ]
        },
        '华模科技': {
          models: [
            {
              ata_code: 'ATA04',
              model_name: 'Aerodynamics',
              latest_version: 'v2.1',
              version_info: {
                '1': { version_id: 'VER011', version: 'v1.0', is_latest: false },
                '2': { version_id: 'VER012', version: 'v2.0', is_latest: false },
                '3': { version_id: 'VER013', version: 'v2.1', is_latest: true }
              }
            },
            {
              ata_code: 'ATA08',
              model_name: 'WeightBalance',
              latest_version: 'v1.5',
              version_info: {
                '1': { version_id: 'VER014', version: 'v1.0', is_latest: false },
                '2': { version_id: 'VER015', version: 'v1.5', is_latest: true }
              }
            }
          ]
        }
      }

      // 返回对应客户的数据，如果没有则返回空数据
      const data = mockData[clientName as keyof typeof mockData] || { models: [] }

      return {
        error: '',
        status: 1,
        data: data
      }
    },
  },

])
