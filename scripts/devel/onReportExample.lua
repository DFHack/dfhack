
local onReport = require 'onReport'

onReport.triggers.onReportExample = function(reportId)
-- print('report '..reportId..' happened!')
 local report = df.report.find(reportId)
 if not report then
  return
 end
-- printall(report)
-- print('\n')
 print(reportId .. ': ' .. df.announcement_type[report["type"]])
 for unitId,_ in pairs(onReport.eventToDwarf[reportId]) do
  print('relevant dwarf: ' .. unitId)
 end
end


