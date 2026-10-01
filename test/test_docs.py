#!/usr/bin/python3
# Copyright (C) 2026 Qore Technologies, s.r.o.
# SPDX-License-Identifier: MIT
"""Check complete overview content and public messaging API links."""
from html.parser import HTMLParser
from pathlib import Path
import sys
import unittest
import xml.etree.ElementTree as ET

BUILD = Path(sys.argv.pop(1)).resolve()


class Links(HTMLParser):
    def __init__(self, path):
        super().__init__()
        self.links = []
        self.feed(path.read_text())

    def handle_starttag(self, tag, attrs):
        attrs = dict(attrs)
        if tag == "a" and "href" in attrs:
            self.links.append(attrs["href"])


class DocumentationTests(unittest.TestCase):
    def test_overview_contains_sections_after_address_examples(self):
        text = (BUILD / "docs/zmq/html/index.html").read_text()
        for section in ("zmqencryption", "zmqreleasenotes"):
            with self.subTest(section=section):
                self.assertIn('id="' + section + '"', text)

    def test_send_and_subscription_links_resolve_to_public_members(self):
        tree = ET.parse(BUILD / "zmq.tag")
        for method in ("send", "subscribe"):
            with self.subTest(method=method):
                targets = {m.findtext("anchorfile") + "#" + m.findtext("anchor")
                           for m in tree.findall(".//member") if m.findtext("name") == method}
                self.assertTrue(targets, method)
                links = set()
                for path in (BUILD / "docs/zmq/html").glob("classQore_1_1ZMQ_1_1ZSocket*.html"):
                    links.update(Links(path).links)
                self.assertTrue(targets.intersection(links), method)


if __name__ == "__main__":
    unittest.main()
