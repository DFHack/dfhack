--onReport.lua
--author expwnent
--contains the "ON_REPORT" event: triggered when there is a new report in df.global.world.status.reports

--example
--local onReport = require 'plugins.onReport'
--onReport.triggers.someName = function (reportId)
-- --do stuff with that id
--end

local _ENV = mkmodule('onReport')
local utils = require 'utils'
local repeatUtil = require 'plugins.repeatUtil'

lastReport = lastReport or -1
triggers = triggers or {}

monitorFrequency = monitorFrequency or nil
eventToDwarf = eventToDwarf or {}

function updateEventToDwarf(reportId)
 if not eventToDwarf[reportId] then
  eventToDwarf[reportId] = {}
 end
 for _,unit in ipairs(df.global.world.units.all) do
  for _,reportType in ipairs(unit.reports.log) do
   for _,report in ipairs(reportType) do
    if report == reportId then
     eventToDwarf[reportId][unit.id] = true
    end
   end
  end
 end
end

function monitor()
 local reports = df.global.world.status.reports
 if df.global.world.status.next_report_id-1 <= lastReport then
  return
 end
-- if #reports == 0 or reports[#reports-1].id <= lastReport then
--  return
-- end
 _,_,start = utils.binsearch(reports,lastReport,"id")
 while start < #reports and reports[start].id <= lastReport do
  start = start+1
 end
 for i=start,#reports-1,1 do
  updateEventToDwarf(reports[i].id)
  for _,callBack in pairs(triggers) do
   callBack(reports[i].id)
  end
  lastReport = reports[i].id
 end
end

monitorEvery = function(n)
 if n <= 0 then
  print('cannot monitor onReport every '..n..' ticks.')
  return
 end
 if monitorFrequency and monitorFrequency < n then
  print('NOT decreasing frequency of onReport monitoring from every '..monitorFrequency..' ticks to every '..n..' ticks')
  return
 end
 print('monitor onReport every '..n..' ticks')
 monitorFrequency = n
 repeatUtil.scheduleEvery('onReportMonitoring', n, 'ticks', monitor)
end

monitorEvery(1)

return _ENV

