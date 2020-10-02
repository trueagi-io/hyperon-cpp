def inteprep_until_result(target, kb):
    result = None
    while not result:
        result = target.interpret_step(kb)
    return result

