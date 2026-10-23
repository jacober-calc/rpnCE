"""Local documentation links and published image references."""
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parent.parent


def documentation_links(root=ROOT):
    pages = [root / 'README.md', *(root / 'docs').rglob('*.md'),
             *(root / 'tests').rglob('*.md')]
    for page in pages:
        text = page.read_text()
        targets = re.findall(r'!?\[[^\]]*\]\(([^\s)]+)(?:\s+"[^"]*")?\)', text)
        targets += re.findall(r'<img\s[^>]*src="([^"]+)"', text)
        for target in targets:
            if '://' in target or target.startswith(('#', 'mailto:')):
                continue
            target = target.split('#', 1)[0]
            if target:
                yield page, (page.parent / target).resolve()


def referenced_assets(root=ROOT):
    assets = (root / 'assets').resolve()
    return {target for _, target in documentation_links(root) if assets in target.parents}


def unused_assets(root=ROOT):
    images = {path.resolve() for path in (root / 'assets').rglob('*')
              if path.is_file() and path.suffix.lower() in
              ('.png', '.jpg', '.jpeg', '.gif', '.svg', '.webp')}
    return sorted(images - referenced_assets(root))
