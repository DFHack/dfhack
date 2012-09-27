# repair him/her. use this when you see "unmovable dwarves" hospital bugs.

# I feel need to implement code of clearing treatment requests.

repairhim = lambda { |u|
    # dirty
    if u.body.wounds.count > 0 then
      u.body.wounds = []
      puts "supermedic: cleared all wounds."
    end
    if u.status2.able_stand < 2 then
      u.status2.able_stand = 2
      puts "supermedic: repaired lost stand ability."
    end
    if u.status2.able_stand_impair < 2 then
      u.status2.able_stand_impair = 2
      puts "supermedic: repaired impaired stand ability."
    end
    # maybe dirty
    if u.job.current_job.job_type == :Rest then
      u.job.current_job.job_type = :CleanSelf
      puts "supermedic: released from 'Rest' job."
    end
}

if him = df.unit_find then
  repairhim[him]
end
