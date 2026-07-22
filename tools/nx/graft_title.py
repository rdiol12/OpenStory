"""Graft the v83 Title button subtrees into the current UI.nx."""
src = open('nx_repack.py').read()
src = src.replace("V83 = 'v83ws'", "V83 = 'v83title'")
src = src.replace("ws = find(root, [b'Login.img', b'WorldSelect'])",
                  "ws = find(root, [b'Login.img', b'Title'])")
src = src.replace("assert ws is not None, 'WorldSelect not found'",
                  "assert ws is not None, 'Title not found'")
src = src.replace(
    "REPLACE = ['scroll', 'BtWorld', 'world', 'channel', 'chBackgrn', 'BtGoworld', 'BtViewAll', 'BtViewChoice', 'alert']",
    "REPLACE = ['BtLogin', 'BtGuestLogin', 'BtLoginIDSave', 'BtLoginIDLost', 'BtPasswdLost', "
    "'BtEmailLost', 'BtEmailSave', 'BtNew', 'BtHomePage', 'BtQuit', 'check']")
src = src.replace("print('WorldSelect children now:'", "print('Title children now:'")
exec(src)
