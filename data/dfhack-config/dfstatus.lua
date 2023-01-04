-- dfstatus config
-- the dfstatus script can be found in hack/scripts/gui/
--[[
The following variables can be set to true/false to enable/disable categories (all true by default)
* drink
* wood
* fuel
* prepared_meals
* tanned_hides
* cloth
* metals

Example:
drink = false
fuel = true

To add metals:
* metal 'IRON'
* metals "GOLD" 'SILVER'
* metal('COPPER')
* metals("BRONZE", 'HORN_SILVER')
Use '-' for a blank line:
* metal '-'
]]

metals 'IRON' 'PIG_IRON' 'STEEL'
metals '-'
metals 'GOLD' 'SILVER' 'COPPER'
