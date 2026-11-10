"""All spoken text.  A dialog is a list of pages; a page is up to 3 lines of at most 18 characters."""

LINE = 18
DIALOGS = {
    # signs and objects
    "SIGN_FARM": [["EAST - Village", "SOUTH - Owsley", "Fork Lake"]],
    "BOOKS": [["Farmer's Almanac:", "Water the crops,", "shoo the crows."]],
    "LOCKED": [["The door's locked.", "Daisy's mama is", "out visiting."]],
    "BOARD": [["NOTICE BOARD", "Lost hen: answers", "to Peep. Ask him."]],
    "WELL": [["A deep old well.", "The water is cold", "and sweet."]],
    "CLOSED": [["ROAD CLOSED", "The bridge washed", "out. Back soon!"]],
    "CABIN": [["Gus's cabin.", "Nobody home. He's", "fishing out back."]],
    "SIGN_LAKE": [["OWSLEY FORK LAKE", "Fishing allowed.", "Be kind to ducks."]],
    "POND": [["NO SWIMMING", "Gus's fishing pond", "Mind the dock!"]],
    "SLEEP": [["Zzz...", "A new day begins", "in the Holler."]],
    # Mamaw Pearl (farmhouse)
    "PEARL_INTRO": [
        ["Well hey there,", "sugar! Welcome", "to the Holler."],
        ["Keep them piggies", "and hens happy,", "ya hear?"],
        ["Hold A to water", "crops. Tap A to", "harvest and feed."],
    ],
    "PEARL_T1": [["The arcade is all", "yours, hon. Try", "Dirt Road Brawler."]],
    "PEARL_T2": [["Air hockey is fun", "after chores. Go", "on, have a game."]],
    "PEARL_T3": [["Tuckered out? The", "bed is yonder.", "Sleep til sunup."]],
    "PEARL_T4": [["Sell extra crops", "at Hobbs' store.", "East trail, hon."]],
    "PEARL_T5": [["SELECT swaps you", "with your farm", "buddy, sugar."]],
    "PEARL_T6": [["Quit a game early?", "START in hockey,", "SELECT in brawler."]],
    "PEARL_T7": [["START opens your", "Breeding Book.", "Pair hens + roos."]],
    # Hobbs (store)
    "HOBBS_HELLO": [["Howdy! Hobbs'", "General Store.", "What'll it be?"]],
    # Daisy (village) wants cucumbers
    "DAISY_ASK": [
        ["Hi! I'm Daisy.", "Mama wants to make", "cucumber salad."],
        ["Could you bring me", "3 cucumbers? I'll", "pay 12 gold!"],
    ],
    "DAISY_WAIT": [["3 cucumbers for", "Mama's salad,", "please!"]],
    "DAISY_DONE": [["Oh thank you!", "Here's 12 gold", "for your trouble."]],
    "DAISY_AFTER": [["Mama says you grow", "the best cucumbers", "around!"]],
    # Wendell (village) wants corn
    "WENDELL_ASK": [
        ["Name's Wendell.", "My hens are plumb", "out of corn."],
        ["Bring me 4 ears of", "corn and I'll pay", "you 15 gold."],
    ],
    "WENDELL_WAIT": [["Four ears of corn,", "that's all I ask.", "Hens are hungry!"]],
    "WENDELL_DONE": [["Bless you, friend!", "Here's 15 gold.", "Hens say thanks."]],
    "WENDELL_AFTER": [["Them hens are lay-", "in' like crazy.", "Much obliged!"]],
    # Old Gus (creek)
    "GUS_T1": [["Fish ain't biting.", "Never do, come to", "think of it."]],
    "GUS_T2": [["Crows after your", "crops? Spray 'em", "with water. Shoo!"]],
    "GUS_T3": [["Hold A to spray.", "It wets the crops", "and scares crows."]],
    "GUS_T4": [["Guinea pigs love", "cucumber, lettuce,", "peppers and hay."]],
    "GUS_T5": [["Chickens want corn", "Feed 'em when ya", "see the bubble."]],
}


def check(font_chars):
    for name, pages in DIALOGS.items():
        for page in pages:
            assert 1 <= len(page) <= 3, name
            for line in page:
                assert len(line) <= LINE, f"{name}: '{line}' is {len(line)} chars"
                for ch in line:
                    assert ch in font_chars, f"{name}: character {ch!r} not in font"


def build():
    """-> (names in index order, blob). Index: n * (u16 first page, u8 page count); pages are 3 x 18 bytes."""
    names = list(DIALOGS)
    index, pages = bytearray(), bytearray()
    page_no = 0
    for n in names:
        index += page_no.to_bytes(2, "little") + bytes([len(DIALOGS[n])])
        for page in DIALOGS[n]:
            for i in range(3):
                line = page[i] if i < len(page) else ""
                pages += line.ljust(LINE).encode("ascii")
            page_no += 1
    return names, bytes(index) + bytes(pages)
