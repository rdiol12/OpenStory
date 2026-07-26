#!/usr/bin/env python3
"""ship_factory_weapon.py — ship a factory-built weapon bundle (build_weapon.py output) to the live game.

Bundle dir must contain: weapon.png (96x96 canonical, grip pinned at 48,80 by construction),
icon.png (32x32), meta.json (donor + nodes/info stats). The canonical PNG ships VERBATIM into
default/weapon via ai_weapon_embed.js (same grip convention: origin(0,0), map/grip=(48,80)).

Usage: ship_factory_weapon.py <bundle_dir> --name "Name" [--desc D] [--type 130..149]
                              [--req-level N] [--commit]
Without --commit prints the plan JSON. On success prints {"itemId": N}.
"""
import argparse, glob, json, os, subprocess, sys
import xml.etree.ElementTree as ET

COSMIC = "/opt/Cosmic"
DASH = "/opt/augurms-ref/dashboard"
TOOL = "/opt/cosmic-creature-gen"
W = f"{COSMIC}/wz/Character.wz/Weapon"
SF = f"{COSMIC}/wz/String.wz/Eqp.img.xml"
TYPE_DONOR = {130: 1302000, 131: 1312000, 132: 1322000, 137: 1372000,
              138: 1382000, 140: 1402000, 143: 1432000, 144: 1442000}
KEEP_STR = {"islot", "vslot", "afterImage", "sfx"}


def alloc_id(prefix):
    used = {int(os.path.basename(f)[1:8]) for f in glob.glob(f"{W}/0*.img.xml")}
    r = ET.parse(SF).getroot()
    sec = r.find('./imgdir[@name="Eqp"]/imgdir[@name="Weapon"]')
    us = {int(c.get("name")) for c in sec.findall("./imgdir")} if sec is not None else set()
    return next(i for i in range(prefix * 1000 + 100, prefix * 1000 + 999)
                if i not in used and i not in us)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("bundle_dir")
    ap.add_argument("--name", required=True)
    ap.add_argument("--desc", default="")
    ap.add_argument("--type", type=int, default=None, choices=sorted(TYPE_DONOR))
    ap.add_argument("--req-level", type=int, default=None)
    ap.add_argument("--commit", action="store_true")
    args = ap.parse_args()

    b = os.path.abspath(args.bundle_dir)
    meta = json.load(open(os.path.join(b, "meta.json")))
    canonical = os.path.join(b, "weapon.png")
    icon = os.path.join(b, "icon.png")
    for p in (canonical, icon):
        if not os.path.exists(p):
            sys.exit(f"missing {p}")
    from PIL import Image
    if Image.open(canonical).size != (96, 96):
        sys.exit("weapon.png must be the 96x96 canonical (grip at 48,80)")

    donor = TYPE_DONOR[args.type] if args.type is not None else int(meta["donor"])
    donor_xml = f"{W}/0{donor}.img.xml"
    if not os.path.exists(donor_xml):
        sys.exit(f"donor XML not found: {donor_xml}")
    prefix = donor // 1000

    info = meta["nodes"]["info"]
    ints = {k: v for k, v in info.items() if isinstance(v, int)}
    strs = {k: v for k, v in info.items() if isinstance(v, str) and k in KEEP_STR}
    if args.req_level is not None:
        ints["reqLevel"] = args.req_level
    ints.setdefault("tuc", 7)

    if args.commit:
        from wz_lock import acquire_wz_lock
        acquire_wz_lock()
    item_id = alloc_id(prefix)

    plan = {"itemId": item_id, "name": args.name, "desc": args.desc, "donor": donor,
            "band": [prefix * 1000 + 100, prefix * 1000 + 998], "canonical": canonical,
            "icon": icon, "infoInts": ints, "infoStrings": strs, "dryRun": not args.commit}
    if not args.commit:
        print(json.dumps(plan, indent=1))
        return

    print(f"ship_factory_weapon: {args.name} id={item_id} donor={donor}", file=sys.stderr)
    r = ET.parse(donor_xml).getroot()
    r.set("name", f"0{item_id}.img")
    inf = next(c for c in r if c.get("name") == "info")
    for k, v in ints.items():
        e = inf.find(f'./int[@name="{k}"]')
        if e is None:
            e = ET.SubElement(inf, "int")
            e.set("name", k)
        e.set("value", str(int(v)))
    open(f"{W}/0{item_id}.img.xml", "w").write(
        '<?xml version="1.0" encoding="UTF-8" standalone="yes"?>\n' + ET.tostring(r, encoding="unicode"))

    rr = ET.parse(SF).getroot()
    sec = rr.find('./imgdir[@name="Eqp"]/imgdir[@name="Weapon"]')
    if sec.find(f'./imgdir[@name="{item_id}"]') is None:
        e = ET.SubElement(sec, "imgdir")
        e.set("name", str(item_id))
        ET.SubElement(e, "string", {"name": "name", "value": args.name})
        if args.desc:
            ET.SubElement(e, "string", {"name": "desc", "value": args.desc})
        open(SF, "w").write(
            '<?xml version="1.0" encoding="UTF-8" standalone="yes"?>\n' + ET.tostring(rr, encoding="unicode"))

    espec = {"itemId": item_id, "canonical": canonical, "iconPath": icon,
             "infoInts": ints, "infoStrings": strs, "name": args.name, "desc": args.desc}
    sp = f"/tmp/factoryweapon_{item_id}.json"
    open(sp, "w").write(json.dumps(espec))
    env = dict(os.environ, NODE_PATH=f"{DASH}/node_modules")
    subprocess.run(["node", "--max-old-space-size=4096", f"{TOOL}/ai_weapon_embed.js", sp],
                   check=True, env=env)
    print(json.dumps({"itemId": item_id, "name": args.name, "donor": donor}))


if __name__ == "__main__":
    sys.path.insert(0, TOOL)
    main()
