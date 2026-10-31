//////////////////////////////////////////////////////////////////////
// Fix inherited member descriptions that don't collapse (Doxygen bug) //
//////////////////////////////////////////////////////////////////////
// Doxygen omits the 'inherit <group>' classes from a memdesc row whenever
// that member also gets a "More..." link (i.e. it has its own detailed
// documentation section, which happens for every documented enum). Since
// dynsections.js's toggleInherit() only shows/hides rows matching
// 'tr.inherit.<group>', and the default stylesheet only hides '.inherit'
// rows, an unclassed memdesc row is always visible, even when its
// "inherited from" section is collapsed. Recover the missing classes from
// the memdesc row's memitem sibling, which is always classed correctly.
document.querySelectorAll('tr[class^="memdesc"]').forEach(memdesc => {
    if (memdesc.classList.contains('inherit')) {
        return;
    }

    const memitem = memdesc.previousElementSibling;
    if (!memitem || !memitem.classList.contains('inherit')) {
        return;
    }

    memitem.classList.forEach(cls => {
        if (!cls.startsWith('memitem:')) {
            memdesc.classList.add(cls);
        }
    });
});

//////////////////////////////
// Highlight diff fragments //
//////////////////////////////
// Doxygen has no diff parser, so ```diff blocks are emitted as plain lines
// with the language tag discarded. Detect them by content instead: every
// non-empty line starts with '+', '-', or ' ', and at least one is a change.
document.querySelectorAll('div.fragment').forEach(fragment => {
    const lines = [...fragment.querySelectorAll(':scope > div.line')];
    const nonEmpty = lines.filter(line => line.textContent.length > 0);

    if (!nonEmpty.every(line => /^[+\- ]/.test(line.textContent)) ||
            !nonEmpty.some(line => /^[+-]/.test(line.textContent))) {
        return;
    }

    lines.forEach(line => {
        if (line.textContent.startsWith('+')) {
            line.classList.add('diff-add');
        } else if (line.textContent.startsWith('-')) {
            line.classList.add('diff-del');
        }
    });
});

////////////////////////////
// Remove "More..." Links //
////////////////////////////

document.querySelectorAll('a').forEach(link => {
    if (link.textContent.trim() === 'More...') {
        link.remove();
    }
});
