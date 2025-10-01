def hex_to_char_array(hex_value):
    # Remove any leading "0x" if present
    hex_value = hex_value.lstrip("0x")

    # Ensure that the hex value has an even number of characters
    if len(hex_value) % 2 != 0:
        hex_value = "0" + hex_value

    # Convert hex value to bytes
    byte_values = bytes.fromhex(hex_value)

    # Generate the C declaration of the char array
    c_declaration = "char array[] = {"
    for byte in byte_values:
        c_declaration += f"0x{byte:02x}, "
    c_declaration = c_declaration.rstrip(", ")
    c_declaration += "};"

    return c_declaration

# Example usage
hex_value = "d6d3fe121c160419844238e3d8c73ca9e09eaee7c7efea54a10100f1b4b846dd" 
c_declaration = hex_to_char_array(hex_value)
print("C Declaration:")
print(c_declaration)
