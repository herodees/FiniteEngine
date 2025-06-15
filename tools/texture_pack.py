import os
import json
import math
from PIL import Image
from typing import List, Dict, Tuple, Optional

class TextureAtlasPacker:
    def __init__(self, input_folder: str, output_path: str, atlas_name: str, 
                 max_size: Tuple[int, int] = (4096, 4096), padding: int = 2,
                 power_of_two: bool = True, recursive: bool = False):
        """
        Initialize the texture atlas packer with recursive search support.
        
        Args:
            input_folder: Path to folder containing images to pack
            output_path: Directory where output files will be saved
            atlas_name: Base name for output files (no extension)
            max_size: Maximum dimensions for the atlas (width, height)
            padding: Number of pixels padding around each image
            power_of_two: Whether to use power-of-two dimensions
            recursive: Whether to search subfolders recursively
        """
        self.input_folder = input_folder
        self.output_path = output_path
        self.atlas_name = atlas_name
        self.max_width, self.max_height = max_size
        self.padding = padding
        self.power_of_two = power_of_two
        self.recursive = recursive
        self.images = []
        self.atlas = None
        self.metadata = {
            'atlas_image': f'{atlas_name}.png',
            'sprites': {},
            'atlas_size': (0, 0),
            'settings': {
                'padding': padding,
                'power_of_two': power_of_two,
                'recursive': recursive
            }
        }

    def load_images(self) -> None:
        """Load all images from the input folder (and subfolders if recursive)."""
        supported_extensions = ('.png', '.jpg', '.jpeg', '.bmp', '.gif', '.tga')
        
        if self.recursive:
            # Walk through all subdirectories
            for root, _, files in os.walk(self.input_folder):
                for filename in files:
                    if filename.lower().endswith(supported_extensions):
                        self._load_image_file(root, filename)
        else:
            # Only process top-level directory
            for filename in os.listdir(self.input_folder):
                if filename.lower().endswith(supported_extensions):
                    self._load_image_file(self.input_folder, filename)

    def _load_image_file(self, directory: str, filename: str) -> None:
        """Helper method to load an individual image file."""
        filepath = os.path.join(directory, filename)
        try:
            img = Image.open(filepath)
            if img.mode != 'RGBA':
                img = img.convert('RGBA')
            
            # Create unique name based on relative path
            rel_path = os.path.relpath(directory, self.input_folder)
            if rel_path == '.':
                name = os.path.splitext(filename)[0]
            else:
                # Replace path separators with underscores
                path_part = rel_path.replace(os.sep, '_')
                name = f"{path_part}_{os.path.splitext(filename)[0]}"
            
            self.images.append({
                'name': name,
                'image': img,
                'size': img.size,
                'original_path': os.path.join(directory, filename)
            })
        except Exception as e:
            print(f"Failed to load image {filename}: {e}")

    def calculate_total_area(self) -> Tuple[int, int]:
        """Calculate the total area needed for all images plus padding."""
        total_area = 0
        max_width = 0
        max_height = 0
        
        for img_data in self.images:
            w, h = img_data['size']
            total_area += (w + self.padding * 2) * (h + self.padding * 2)
            max_width = max(max_width, w + self.padding * 2)
            max_height = max(max_height, h + self.padding * 2)
        
        return total_area, max_width, max_height

    def find_optimal_atlas_size(self) -> Tuple[int, int]:
        """
        Find the smallest possible atlas size that can fit all images.
        Uses either power-of-two dimensions or tries to find the most efficient rectangle.
        """
        total_area, max_width, max_height = self.calculate_total_area()
        
        if self.power_of_two:
            # Try common power-of-two sizes
            sizes = []
            size = 32
            while size <= min(self.max_width, self.max_height):
                sizes.append(size)
                size *= 2
            
            # Try square textures first
            for size in sizes:
                if size * size >= total_area:
                    return (size, size)
            
            # Try rectangular textures if square isn't enough
            for w in sizes:
                for h in sizes:
                    if w * h >= total_area and w >= max_width and h >= max_height:
                        return (w, h)
        else:
            # Non-power-of-two approach
            min_side = int(math.sqrt(total_area))
            w = h = min_side
            w = max(w, max_width)
            h = max(h, max_height)
            
            aspect_ratio = max_width / max_height if max_height > 0 else 1
            if aspect_ratio > 1.5:
                w = min(int(h * aspect_ratio), self.max_width)
            elif aspect_ratio < 0.67:
                h = min(int(w / aspect_ratio), self.max_height)
            
            w = min(w, self.max_width)
            h = min(h, self.max_height)
            
            return (w, h)
        
        # Fallback to max size if no suitable size found
        print(f"Warning: Could not find suitable size within max {self.max_width}x{self.max_height}")
        return (self.max_width, self.max_height)

    def pack_images(self, atlas_width=0, atlas_height=0, attempt=1) -> bool:
        """Pack images into atlas with recursive size adjustment if needed."""
        if not self.images:
            print("No images to pack!")
            return False

        self.images.sort(key=lambda x: x['size'][0] * x['size'][1], reverse=True)

        if atlas_width <= 0 or atlas_height <= 0:
            atlas_width, atlas_height = self.find_optimal_atlas_size()
        print(f"Attempt {attempt}: Packing into {atlas_width}x{atlas_height} atlas")

        # Validate packing
        current_x = 0
        current_y = 0
        row_height = 0
        success = True

        for img_data in self.images:
            width, height = img_data['size']
            padded_width = width + self.padding * 2
            padded_height = height + self.padding * 2

            if current_x + padded_width > atlas_width:
                current_x = 0
                current_y += row_height
                row_height = 0

                if current_y + padded_height > atlas_height:
                    success = False
                    break

            current_x += padded_width
            row_height = max(row_height, padded_height)

        if not success:
            if attempt >= 3:
                print("Max retry attempts reached")
                return False
                
            new_width = min(atlas_width * 2, self.max_width)
            new_height = min(atlas_height * 2, self.max_height)
            
            if new_width == atlas_width and new_height == atlas_height:
                print("Already at max size, cannot expand further")
                return False
                
            return self.pack_images(new_width, new_height, attempt + 1)

        # Perform actual packing
        self.atlas = Image.new('RGBA', (atlas_width, atlas_height), (0, 0, 0, 0))
        self.metadata['atlas_size'] = (atlas_width, atlas_height)

        current_x = 0
        current_y = 0
        row_height = 0

        for img_data in self.images:
            img = img_data['image']
            width, height = img.size
            padded_width = width + self.padding * 2
            padded_height = height + self.padding * 2

            if current_x + padded_width > atlas_width:
                current_x = 0
                current_y += row_height
                row_height = 0

            x = current_x + self.padding
            y = current_y + self.padding
            
            self.atlas.paste(img, (x, y))
            
            self.metadata['sprites'][img_data['name']] = {
                'x': x,
                'y': y,
                'width': width,
                'height': height,
                'rotated': False,
                'original_path': img_data.get('original_path', '')
            }

            current_x += padded_width
            row_height = max(row_height, padded_height)

        return True

    def save_atlas(self) -> None:
        """Save the atlas image and metadata."""
        if not os.path.exists(self.output_path):
            os.makedirs(self.output_path)

        png_path = os.path.join(self.output_path, f'{self.atlas_name}.png')
        self.atlas.save(png_path, 'PNG')
        print(f"Saved atlas image to {png_path}")

        json_path = os.path.join(self.output_path, f'{self.atlas_name}.json')
        with open(json_path, 'w') as f:
            json.dump(self.metadata, f, indent=2)
        print(f"Saved atlas metadata to {json_path}")

    def process(self) -> bool:
        """Run the full packing process."""
        self.load_images()
        if not self.images:
            print("No valid images found in input folder.")
            return False
        
        success = self.pack_images()
        if not success:
            return False
        
        self.save_atlas()
        return True


if __name__ == "__main__":
    import argparse

    parser = argparse.ArgumentParser(description='Pack images into a texture atlas with recursive search support.')
    parser.add_argument('input_folder', help='Folder containing images to pack')
    parser.add_argument('output_folder', help='Folder to save the atlas files')
    parser.add_argument('--name', default='atlas', help='Base name for output files')
    parser.add_argument('--max_width', type=int, default=4096, help='Maximum atlas width')
    parser.add_argument('--max_height', type=int, default=4096, help='Maximum atlas height')
    parser.add_argument('--padding', type=int, default=2, help='Padding between images')
    parser.add_argument('--no_pot', action='store_false', dest='power_of_two', 
                       help='Disable power-of-two dimensions')
    parser.add_argument('--recursive', action='store_true',
                       help='Search for images in subfolders recursively')

    args = parser.parse_args()

    packer = TextureAtlasPacker(
        input_folder=args.input_folder,
        output_path=args.output_folder,
        atlas_name=args.name,
        max_size=(args.max_width, args.max_height),
        padding=args.padding,
        power_of_two=args.power_of_two,
        recursive=args.recursive
    )

    if packer.process():
        print("Texture atlas created successfully!")
        print(f"Final atlas size: {packer.metadata['atlas_size'][0]}x{packer.metadata['atlas_size'][1]}")
        print(f"Packed {len(packer.images)} images")
    else:
        print("Failed to create texture atlas.")
    