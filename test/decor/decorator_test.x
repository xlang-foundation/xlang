# ============================================================
# Decorator 1: LogDecorator — logs calls before and after execution
# ============================================================
def LogDecorator():
    print("[LogDecorator] Initialized")

    def log_wrapper(info):
        print("[LogDecorator] >>> Entering")

        # 'this' refers to the current wrapper function object
        # Use getattr("origin") to get the original decorated function
        original_func = this.getattr("origin")

        # Call the original function and capture its return value
        result = original_func(info)

        print("[LogDecorator] <<< Exiting")
        return result

    return log_wrapper


# ============================================================
# Decorator 2: RepeatDecorator — repeats the call N times
# ============================================================
def RepeatDecorator(repeat_expr):
    print("[RepeatDecorator] Initialized with expression:", repeat_expr)

    def repeat_wrapper(info):
        print("[RepeatDecorator] >>> Entering")

        # Get stored repeat count
        repeat_count = this.getattr("repeat_count")

        # Get the original decorated function
        original_func = this.getattr("origin")

        # Call the original function multiple times
        result = None
        for i in range(repeat_count):
            print("[RepeatDecorator] Executing iteration ${i}/${repeat_count}")
            result = original_func(info + ", run ${i}")

        print("[RepeatDecorator] <<< Exiting")
        return result  # Always return the last call's result

    # In XLang, decorator parameters are *expressions*.
    # Use () to evaluate and obtain the actual value.
    evaluated_count = repeat_expr()

    # Store evaluated repeat count inside the wrapper
    repeat_wrapper.setattr("repeat_count", evaluated_count)

    return repeat_wrapper


# ============================================================
# Example target function
# ============================================================
@LogDecorator()
@RepeatDecorator(3)
def say_hello(info):
    print("[say_hello] Hello from XLang: ${info}")
    return info


# ============================================================
# Execute and log result
# ============================================================
result = say_hello("OK, from Log")
print("[Main] Returned value: ${result}")
print("[Main] Done.")
