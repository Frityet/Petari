import json

def on_visibility(frame, location, internal_dict):
    target = frame.GetThread().GetProcess().GetTarget()
    symbol = target.FindSymbols('SDL_CursorVisible_REAL').GetContextAtIndex(0).GetSymbol()
    address = symbol.GetStartAddress().GetLoadAddress(target)
    visible = frame.EvaluateExpression('((bool(*)())0x%x)()' % address)
    print('[cursor-probe] ' + json.dumps({'sdl_cursor_visible': visible.GetValue(), 'query_error': str(visible.GetError())}))
    return False
