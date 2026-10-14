/**
 * @brief Validate optional capture rectangles without losing precision for native integer limits.
 * @param value Capture rectangle text in x,y,width,height format.
 * @return Whether the text is empty or represents a nonnegative rectangle with positive area.
 */
export function validCaptureCrop(value: string): boolean {
  const text = value.trim();
  if (!text) return true;
  if (!/^\d+,\d+,\d+,\d+$/.test(text)) return false;
  const parts = text.split(',').map(Number);
  return (
    parts.every((part) => Number.isInteger(part) && part <= 2147483647) &&
    parts[2] > 0 &&
    parts[3] > 0
  );
}
