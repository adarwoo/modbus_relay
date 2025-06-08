from PIL import Image, ImageDraw
import numpy as np
import imageio

# Parameters
width, height = 200, 200  # Image dimensions
led_radius = 50           # LED size
duration = 1.0            # Duration of one cycle (seconds)
frames_per_second = 20    # Frame rate
total_duration = 2.0      # Total animation duration (seconds)
output_file = "flashing_led.gif"

# Calculate number of frames
num_frames = int(total_duration * frames_per_second)
frames_per_flash = int(frames_per_second / 2)  # 2Hz = 2 flashes per second

# Colors
led_on_color = (255, 0, 0)    # Red
led_off_color = (50, 0, 0)    # Dark red
background_color = (0, 0, 0)  # Black

# Create frames
frames = []
for i in range(num_frames):
    # Create a new image
    img = Image.new('RGB', (width, height), background_color)
    draw = ImageDraw.Draw(img)

    # Determine if LED should be on or off (2Hz flashing)
    # Flash stays on for half the flash period (25% duty cycle)
    flash_phase = i % frames_per_flash
    led_on = flash_phase < frames_per_flash / 4

    # Draw the LED
    led_color = led_on_color if led_on else led_off_color
    draw.ellipse([(width//2 - led_radius, height//2 - led_radius),
                  (width//2 + led_radius, height//2 + led_radius)],
                 fill=led_color, outline=(100, 100, 100))

    # Add reflection highlight to make it look more like an LED
    if led_on:
        highlight_radius = led_radius // 3
        draw.ellipse([(width//2 - highlight_radius, height//2 - highlight_radius),
                      (width//2 + highlight_radius, height//2 + highlight_radius)],
                     fill=(255, 150, 150))

    # Convert to numpy array for imageio
    frames.append(np.array(img))

# Save as GIF
imageio.mimsave(output_file, frames, fps=frames_per_second, loop=0)

print(f"Successfully created {output_file} with {num_frames} frames")