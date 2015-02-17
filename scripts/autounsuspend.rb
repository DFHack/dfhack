class AutoUnsuspend
    attr_accessor :running

    def process
        count = 0
        df.world.job_list.each { |job|
            if job.job_type == :ConstructBuilding and job.flags.suspend and df.map_tile_at(job).designation.flow_size <= 1
                job.flags.suspend = false
                count += 1
            end
        }
        if count > 0
            puts "Unsuspended #{count} job(s)."
            df.process_jobs = true
        end
    end

    def start
        @running = true
        @onupdate = df.onupdate_register('autounsuspend', 5) { process if @running }
    end

    def stop
        @running = false
        df.onupdate_unregister(@onupdate)
    end
end

case $script_args[0]
when 'start'
    $AutoUnsuspend ||= AutoUnsuspend.new
    $AutoUnsuspend.start

when 'end', 'stop'
    $AutoUnsuspend.stop

else
    puts $AutoUnsuspend && $AutoUnsuspend.running ? 'Running.' : 'Stopped.'
end
