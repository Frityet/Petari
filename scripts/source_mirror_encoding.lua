-- Strict source-boundary normalization for the explicit compile-time encoding API.
-- All non-wrapper bytes, including comments and whitespace, remain unchanged.
local function identifier(value)
    return value ~= "" and value:match("[%w_]") ~= nil
end

local function skip_space(source, position)
    while position <= #source and source:sub(position, position):match("%s") do position = position + 1 end
    return position
end

local function comment_end(source, position)
    local prefix = source:sub(position, position + 1)
    if prefix == "/*" then
        local ending = source:find("*/", position + 2, true)
        return ending and ending + 2 or #source + 1
    end
    if prefix ~= "//" then return position end
    local ending = position + 2
    repeat
        ending = source:find("\n", ending, true)
        if not ending then return #source + 1 end
        local previous = ending - 1
        if source:sub(previous, previous) == "\r" then previous = previous - 1 end
        ending = ending + 1
        if source:sub(previous, previous) ~= "\\" then break end
    until ending > #source
    return ending
end

local function skip_trivia(source, position)
    while true do
        position = skip_space(source, position)
        local ending = comment_end(source, position)
        if ending == position then return position end
        position = ending
    end
end

local function literal_end(source, position)
    local start = position
    if source:sub(position, position + 1) == "u8" then position = position + 2
    elseif source:sub(position, position):match("[uUL]") then position = position + 1 end
    local raw = source:sub(position, position) == "R"
    if raw then position = position + 1 end
    local quote = source:sub(position, position)
    if quote ~= '"' and quote ~= "'" then return start end
    position = position + 1
    if raw then
        if quote ~= '"' then return start end
        local opening = source:find("(", position, true)
        if not opening or opening - position > 16 then return #source + 1 end
        local closing = ")" .. source:sub(position, opening - 1) .. '"'
        local ending = source:find(closing, opening + 1, true)
        if not ending then return #source + 1 end
        position = ending + #closing
    else
        while position <= #source do
            local value = source:sub(position, position)
            position = position + 1
            if value == "\\" and position <= #source then position = position + 1
            elseif value == quote then break end
        end
    end
    while identifier(source:sub(position, position)) do position = position + 1 end
    return position
end

function normalize(source)
    local include = '#include "compat/Cp932Literal.hpp"\n'
    if source:sub(1, #include) == include then source = source:sub(#include + 1) end
    local output, cursor = {}, 1
    while cursor <= #source do
        local next_position = comment_end(source, cursor)
        if next_position == cursor then next_position = literal_end(source, cursor) end
        if next_position ~= cursor then
            output[#output + 1] = source:sub(cursor, next_position - 1)
            cursor = next_position
        elseif identifier(source:sub(cursor, cursor)) then
            local ending = cursor + 1
            while identifier(source:sub(ending, ending)) do ending = ending + 1 end
            local matched = false
            if source:sub(cursor, ending - 1) == "CP932" then
                local opening = skip_space(source, ending)
                if source:sub(opening, opening) == "(" then
                    local first = skip_space(source, opening + 1)
                    local last, next_literal = first, first
                    while source:sub(next_literal, next_literal) == '"' do
                        last = literal_end(source, next_literal)
                        if last == next_literal or source:sub(last - 1, last - 1) ~= '"' then break end
                        next_literal = skip_trivia(source, last)
                    end
                    if last > first and source:sub(last - 1, last - 1) == '"' and
                        next_literal == skip_space(source, last) and source:sub(next_literal, next_literal) == ")" then
                        output[#output + 1] = source:sub(first, last - 1)
                        cursor, matched = next_literal + 1, true
                    end
                end
            end
            if not matched then
                output[#output + 1] = source:sub(cursor, ending - 1)
                cursor = ending
            end
        else
            output[#output + 1] = source:sub(cursor, cursor)
            cursor = cursor + 1
        end
    end
    return table.concat(output)
end

return {normalize = normalize}
