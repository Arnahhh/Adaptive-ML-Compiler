def calculate_total(price, quantity):
    subtotal = price * quantity + 2 + 3
    return subtotal


if __name__ == "__main__":
    print(calculate_total(12, 3))
