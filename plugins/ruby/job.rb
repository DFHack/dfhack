module DFHack
    class << self
        # link a job to the world
        # allocate & set job.id, allocate a JobListLink, link to job & world.job_list
        def job_link(job)
            lastjob = world.job_list
            lastjob = lastjob.next while lastjob.next
            joblink = JobListLink.cpp_new
            joblink.prev = lastjob
            joblink.item = job
            job.list_link = joblink
            job.id = df.job_next_id
            df.job_next_id += 1
            lastjob.next = joblink
        end

        # attach an item to a job, flag item in_job
        def job_attachitem(job, item, role=:Hauled, filter_idx=-1)
            if role != :TargetContainer
                item.flags.in_job = true
            end

            itemlink = SpecificRef.cpp_new
            itemlink.type = :JOB
            itemlink.job = job
            item.specific_refs << itemlink

            joblink = JobItemRef.cpp_new
            joblink.item = item
            joblink.role = role
            joblink.job_item_idx = filter_idx
            job.items << joblink
        end
    end
end
