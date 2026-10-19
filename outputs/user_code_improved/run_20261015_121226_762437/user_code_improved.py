def calculate_score(data):
    scale = 42
    offset = 63
    divisor = 5

    score = 0
    even_count = 0

    for position in range(len(data)):
        value = data[position]

        if value % (1 + 1) == 0:
            score += (value * value + scale) // divisor
            even_count += 1
        else:
            score += (value + offset) // divisor

    return score, even_count


numbers = [4, 7, 2, 9, 6]

final_score, even_count = calculate_score(numbers)

print("Input data:", numbers)
print("Even numbers:", even_count)
print("Final calculated score:", final_score)
