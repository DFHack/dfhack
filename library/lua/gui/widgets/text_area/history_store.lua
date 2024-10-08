HistoryStore = defclass(HistoryStore)

local HISTORY_ENTRY = {
    TEXT_BLOCK = 1,
    WHITESPACE_BLOCK = 2,
    BACKSPACE = 2,
    DELETE = 3,
    OTHER = 4
}

HistoryStore.ATTRS{
    history_size = 25,
}

function HistoryStore:init()
    self.past = {}
    self.future = {}
end

function HistoryStore:store(history_entry_type, text, cursor)
    local last_entry = self.past[#self.past]

    if not last_entry or history_entry_type == HISTORY_ENTRY.OTHER or
        last_entry.entry_type ~= history_entry_type then
        table.insert(self.past, {
            entry_type=history_entry_type,
            text=text,
            cursor=cursor
        })
    end

    self.future = {}

    if #self.past > self.history_size then
        table.remove(self.past, 1)
    end
end

function HistoryStore:undo(curr_text, curr_cursor)
    if #self.past == 0 then
        return nil
    end

    local history_entry = table.remove(self.past, #self.past)

    table.insert(self.future, {
        entry_type=HISTORY_ENTRY.OTHER,
        text=curr_text,
        cursor=curr_cursor
    })

    if #self.future > self.history_size then
        table.remove(self.future, 1)
    end

    return history_entry
end

function HistoryStore:redo(curr_text, curr_cursor)
    if #self.future == 0 then
        return true
    end

    local history_entry = table.remove(self.future, #self.future)

    table.insert(self.past, {
        entry_type=HISTORY_ENTRY.OTHER,
        text=curr_text,
        cursor=curr_cursor
    })

    if #self.past > self.history_size then
        table.remove(self.past, 1)
    end

    return history_entry
end

HistoryStore.HISTORY_ENTRY = HISTORY_ENTRY

return HistoryStore
