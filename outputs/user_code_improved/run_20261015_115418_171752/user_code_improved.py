def calculate_sum(limit):
    total = 0

    for number in range(1, limit + 1):
        if number % 2 == 0:
            total = total + number * number

    return total


limit = 10
result = calculate_sum(limit)

print("Limit:", limit)
print("Sum of squares of even numbers:", result)
