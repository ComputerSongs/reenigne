#include "alfe/main.h"
#include "alfe/cga.h"

class GameWindow : public RootWindow
{
public:
    GameWindow()
      : _wisdom(File("wisdom")), _output(&_data, &_sequencer, &_bitmap)
    {
        _output.setConnector(1);          // old composite
        _output.setScanlineProfile(0);    // rectangle
        _output.setHorizontalProfile(0);  // rectangle
        _output.setScanlineWidth(1);
        _output.setScanlineBleeding(2);   // symmetrical
        _output.setHorizontalBleeding(2); // symmetrical
        _output.setZoom(2);
        _output.setHorizontalRollOff(0);
        _output.setHorizontalLobes(4);
        _output.setVerticalRollOff(0);
        _output.setVerticalLobes(4);
        _output.setSubPixelSeparation(1);
        _output.setPhosphor(0);           // colour
        _output.setMask(0);
        _output.setMaskSize(0);
        _output.setAspectRatio(5.0/6.0);
        _output.setOverscan(0);
        _output.setCombFilter(0);         // no filter
        _output.setHue(0);
        _output.setSaturation(100);
        _output.setContrast(100);
        _output.setBrightness(0);
        _output.setShowClipping(false);
        _output.setChromaBandwidth(1);
        _output.setLumaBandwidth(1);
        _output.setRollOff(0);
        _output.setLobes(1.5);
        _output.setPhase(1);

        _regs = -CGAData::registerLogCharactersPerBank;
        _cgaBytes.allocate(0x4000 + _regs);
        Byte* cgaRegisters = &_cgaBytes[_regs];
        cgaRegisters[CGAData::registerLogCharactersPerBank] = 12;
        cgaRegisters[CGAData::registerScanlinesRepeat] = 1;
        cgaRegisters[CGAData::registerHorizontalTotalHigh] = 0;
        cgaRegisters[CGAData::registerHorizontalDisplayedHigh] = 0;
        cgaRegisters[CGAData::registerHorizontalSyncPositionHigh] = 0;
        cgaRegisters[CGAData::registerVerticalTotalHigh] = 0;
        cgaRegisters[CGAData::registerVerticalDisplayedHigh] = 0;
        cgaRegisters[CGAData::registerVerticalSyncPositionHigh] = 0;
        cgaRegisters[CGAData::registerMode] = 9;
        cgaRegisters[CGAData::registerPalette] = 0;
        cgaRegisters[CGAData::registerHorizontalTotal] = 114 - 1;
        cgaRegisters[CGAData::registerHorizontalDisplayed] = 80;
        cgaRegisters[CGAData::registerHorizontalSyncPosition] = 90;
        cgaRegisters[CGAData::registerHorizontalSyncWidth] = 16;
        cgaRegisters[CGAData::registerVerticalTotal] = 128 - 1;
        cgaRegisters[CGAData::registerVerticalTotalAdjust] = 6;
        cgaRegisters[CGAData::registerVerticalDisplayed] = 100;
        cgaRegisters[CGAData::registerVerticalSyncPosition] = 112;
        cgaRegisters[CGAData::registerInterlaceMode] = 2;
        cgaRegisters[CGAData::registerMaximumScanline] = 1;
        cgaRegisters[CGAData::registerCursorStart] = 6;
        cgaRegisters[CGAData::registerCursorEnd] = 7;
        cgaRegisters[CGAData::registerStartAddressHigh] = 0;
        cgaRegisters[CGAData::registerStartAddressLow] = 0;
        cgaRegisters[CGAData::registerCursorAddressHigh] = 0;
        cgaRegisters[CGAData::registerCursorAddressLow] = 0;
        _data.setTotals(238944, 910, 238875);
        _data.change(0, -_regs, _regs + 0x4000, &_cgaBytes[0]);
		_vram = &_cgaBytes[_regs];

        _outputSize = _output.requiredSize();

        add(&_bitmap);
        add(&_animated);

        _animated.setDrawWindow(this);
        _animated.setRate(60);

        _background.allocate(0x10000);
        _foreground.allocate(0x10000);
        _buffer.allocate(0x10000);
        _tiles.allocate(0x10000);
        _tileColumns = 8;
        _tileWidthBytes = _tileColumns << 1;
        _tileRows = 16;
        _bufferStride = 0x100;
        _screenColumns = 80;
		_screenWidthBytes = _screenColumns << 1;
        _screenRows = 100;
        _mapStride = 0x100;
        _horizontalAcceleration = 0x100;
        _verticalAcceleration = 0x100;
        _horizontalMaxVelocity = 0x100;
        _verticalMaxVelocity = 0x100;
		_tilePointers.allocate(0x100);
		Word tilePointer = 0;
		int tileBytes = _tileRows*_tileWidthBytes;
		for (int i = 0; i < 0x100; ++i) {
			_tilePointers[i] = tilePointer;
			tilePointer += tileBytes;
		}

		_tilesPerScreenHorizontally =
			(_screenColumns + 2*_tileColumns - 2) / _tileColumns;
		_tilesPerScreenVertically =
			(_screenRows + 2*_tileRows - 2) / _tileRows;
		// We extend the horizontal transition areas out to the corners.
		// TODO: Decide whether to associate the corners with the horizontal
		// transition areas or the vertical transition areas based on
		// _tilesPerScreenHorizontally/_tileRows and
		// _tilesPerScreenVertically/_tileColumns .
		int maxNodes = ((_tilesPerScreenHorizontally + 3)/2 +
			(_tilesPerScreenVertically + 1)/2)*2;
	    _nodePool.allocate(maxNodes);
		initList(&_freeNodes);
		initList(&_leftNodes);
		initList(&_topNodes);
		initList(&_rightNodes);
		initList(&_bottomNodes);
		for (int i = 0; i < maxNodes; ++i)
			freeNode(&_nodePool[i]);
		Node* n = getNode();
		n->drawn = _tilesPerScreenVertically;
		n->undrawn = 0;
		insertNodeAfter(n, &_leftNodes);
		_leftNodes.drawn = n->drawn;
		n = getNode();
		n->drawn = _tilesPerScreenHorizontally + 2;
		n->undrawn = 0;
		insertNodeAfter(n, &_topNodes);
		_topNodes.drawn = n->drawn;
		n = getNode();
		n->drawn = _tilesPerScreenVertically;
		n->undrawn = 0;
		insertNodeAfter(n, &_rightNodes);
		_rightNodes.drawn = n->drawn;
		n = getNode();
		n->drawn = _tilesPerScreenHorizontally + 2;
		n->undrawn = 0;
		insertNodeAfter(n, &_bottomNodes);
		_bottomNodes.drawn = n->drawn;
		_bufferLeft.allocate(_tilesPerScreenVertically);
		_mapLeft.allocate(_tilesPerScreenVertically);
		_bufferTop.allocate(_tilesPerScreenHorizontally + 2);
		_bufferRight.allocate(_tilesPerScreenVertically);
		_mapRight.allocate(_tilesPerScreenVertically);
		_bufferBottom.allocate(_tilesPerScreenHorizontally + 2);
		_mapBottom.allocate(_tilesPerScreenHorizontally + 2);
		int bufferTileStride = _bufferStride*_tileRows;
		int bufferLeft = bufferTileStride;
		int mapLeft = _mapStride;
		int bufferRight = bufferLeft +
			(_tilesPerScreenHorizontally + 1)*_tileColumns;
		int mapRight = mapLeft + _tilesPerScreenHorizontally + 1;
		for (int i = 0; i < _tilesPerScreenVertically; ++i) {
			_bufferLeft[i] = bufferLeft;
			_mapLeft[i] = mapLeft;
			_bufferRight[i] = bufferRight;
			_mapRight[i] = mapRight;
			bufferLeft += bufferTileStride;
			bufferRight += bufferTileStride;
			mapLeft += _mapStride;
			mapRight += _mapStride;
		}
		int bufferTop = 0;
		int bufferBottom = (_tilesPerScreenVertically + 1)*bufferTileStride;
		int mapBottom = (_tilesPerScreenVertically + 1)*_mapStride;
		for (int i = 0; i < _tilesPerScreenHorizontally + 2; ++i) {
			_bufferTop[i] = bufferTop;
			_bufferBottom[i] = bufferBottom;
			_mapBottom[i] = mapBottom;
			bufferTop += _tileColumns;
			bufferBottom += _tileColumns;
			++mapBottom;
		}

		_transitionCountLeft.allocate(_tileColumns);
		_transitionCountTop.allocate(_tileRows);
		_transitionCountRight.allocate(_tileColumns);
		_transitionCountBottom.allocate(_tileRows);
		for (int i = 0; i < _tileColumns; ++i) {
			_transitionCountLeft[i] =
				(_tileColumns - i)*_tilesPerScreenVertically/_tileColumns;
			_transitionCountRight[i] =
				(1 + i)*_tilesPerScreenVertically/_tileColumns;
		}
		for (int i = 0; i < _tileRows; ++i) {
			_transitionCountTop[i] =
				(_tileRows - i)*(_tilesPerScreenHorizontally + 2)/_tileRows;
			_transitionCountBottom[i] =
				(1 + i)*(_tilesPerScreenHorizontally + 2)/_tileRows;
		}


        for (int i = 0; i < 0x10000; ++i) {
            _background[i] = rand() & 0xff;
            _foreground[i] = rand() & 0xff;
            _tiles[i] = rand() & 0xff;
        }

		// Draw initial screen
		_bufferTL = 0;
		_mapTL = 0;
		int bufferRow = -_tileRows*_bufferStride - _tileWidthBytes;
		int mapRow = -_mapStride - 1;
		for (int y = -1; y < _tilesPerScreenVertically + 1; ++y) {
			int buffer = bufferRow;
			int map = mapRow;
			for (int x = -1; x < _tilesPerScreenHorizontally + 1; ++x) {
				drawTile(buffer, map);
				buffer += _tileWidthBytes;
				++map;
			}
			bufferRow += _bufferStride*_tileRows;
			mapRow += _mapStride;
		}
		_bufferTopLeft = _tileColumns + _tileRows*_bufferStride;

		addUpdateBlock(0, 0, _screenColumns, _screenRows);
		for (auto i : _updateBlocks)
			updateBlock(i);
		_updateBlocks.clear();
	}
    void create()
    {
        setText("CGA game");
        setInnerSize(_outputSize);
        _bitmap.setTopLeft(Vector(0, 0));
        _bitmap.setInnerSize(_outputSize);
        RootWindow::create();
        _animated.start();
    }
    virtual void draw()
    {
        // Time: immediately before active region starts
        // IRQ0 fires to stop VRAM accessing
        // (not emulated here - we assume there's time for all VRAM writes)

        // Time: active region starts
        // CRTC latches start address
        // (VRAM data is also latched here but we don't write to that in
        // active region anyway so it's fine)
        _data.change(0, -_regs, _regs + 0x4000, &_cgaBytes[0]);
        _output.restart();
        _animated.restart();

        // onScreenHandler
        // EOI
        // Switch IRQ0 handler to offScreenHandler
        // Set count for PIT channel 0 to inactive cycles
        // Start foreground sound
        // Set start address:
        _cgaBytes[_regs + CGAData::registerStartAddressHigh] =
            _startAddress >> 8;
        _cgaBytes[_regs + CGAData::registerStartAddressLow] =
            _startAddress & 0xff;
        // Check keyboard
        if (_leftPressed) {
            if (!_rightPressed) {
                // Speed up leftwards
                _xVelocity -= _horizontalAcceleration;
                if (_xVelocity < -_horizontalMaxVelocity)
                    _xVelocity = -_horizontalMaxVelocity;
            }
            // If both left and right are pressed, maintain horizontal velocity
        }
        else {
            if (_rightPressed) {
                // Speed up rightwards
                _xVelocity += _horizontalAcceleration;
                if (_xVelocity > _horizontalMaxVelocity)
                    _xVelocity = _horizontalMaxVelocity;
            }
            else {
                // Slow down
                if (_xVelocity > 0) {
                    _xVelocity -= _horizontalAcceleration;
                    if (_xVelocity < 0)
                        _xVelocity = 0;
                }
                else {
                    _xVelocity += _horizontalAcceleration;
                    if (_xVelocity > 0)
                        _xVelocity = 0;
                }
            }
        }
        if (_upPressed) {
            if (!_downPressed) {
                // Speed up downwards
                _yVelocity -= _verticalAcceleration;
                if (_yVelocity < -_verticalMaxVelocity)
                    _yVelocity = -_verticalMaxVelocity;
            }
            // If both up and down are pressed, maintain vertical velocity
        }
        else {
            if (_downPressed) {
                // Speed up downwards
                _yVelocity += _verticalAcceleration;
                if (_yVelocity > _verticalMaxVelocity)
                    _yVelocity = _verticalMaxVelocity;
            }
            else {
                // Slow down
                if (_yVelocity > 0) {
                    _yVelocity -= _verticalAcceleration;
                    if (_yVelocity < 0)
                        _yVelocity = 0;
                }
                else {
                    _yVelocity += _verticalAcceleration;
                    if (_yVelocity > 0)
                        _yVelocity = 0;
                }
            }
        }
        // Move
        int xSubTileHighOld = _xSubTile >> 8;
        int ySubTileHighOld = _ySubTile >> 8;
        if (_xVelocity > 0) {
            _xSubTile += _xVelocity;
            if ((_xSubTile >> 8) > _tileColumns) {
                _xSubTile -= _tileColumns << 8;
                ++_mapTL;
				setListFilled(&_leftNodes, _tilesPerScreenVertically);
				setListEmpty(&_rightNodes, _tilesPerScreenVertically);
				shiftLeft(&_topNodes);
				shiftLeft(&_bottomNodes);
            }
        }
        else {
            _xSubTile += _xVelocity;
            if ((_xSubTile >> 8) < 0) {
                _xSubTile += _tileColumns << 8;
                --_mapTL;
				setListEmpty(&_leftNodes, _tilesPerScreenVertically);
				setListFilled(&_rightNodes, _tilesPerScreenVertically);
				shiftRight(&_topNodes);
				shiftRight(&_bottomNodes);
            }
        }
        if (_yVelocity > 0) {
            _ySubTile += _yVelocity;
            if ((_ySubTile >> 8) > _tileRows) {
                _ySubTile -= _tileRows << 8;
				_mapTL += _mapStride;
				setListFilled(&_topNodes, _tilesPerScreenHorizontally + 2);
				setListEmpty(&_bottomNodes, _tilesPerScreenHorizontally + 2);
				shiftUp(&_leftNodes);
				shiftUp(&_rightNodes);
            }
        }
        else {
            _ySubTile += _yVelocity;
            if ((_ySubTile >> 8) < 0) {
                _ySubTile += _tileRows << 8;
				_mapTL -= _mapStride;
				setListEmpty(&_topNodes, _tilesPerScreenHorizontally + 2);
				setListFilled(&_bottomNodes, _tilesPerScreenHorizontally + 2);
				shiftDown(&_leftNodes);
				shiftDown(&_rightNodes);
            }
        }
        int deltaX = (_xSubTile >> 8) - xSubTileHighOld;
        int deltaY = (_ySubTile >> 8) - ySubTileHighOld;
		if (deltaX < 0) {
			if (deltaY < 0) {
				// Move up and left
				_startAddress -= _screenColumns + 1;
				_bufferTopLeft -= _screenWidthBytes;
				addUpdateBlock(0, 0, _screenColumns, 1);
				addUpdateBlock(0, 1, 1, _screenRows - 1);
				leftTiles();
				topTiles();
			}
			else {
				if (deltaY > 0) { 
					// Move down and left
					_startAddress += _screenColumns - 1;
					_bufferTopLeft += _screenWidthBytes - 2;
					addUpdateBlock(_screenRows - 1, 0, _screenColumns, 1);
					addUpdateBlock(0, 0, 1, _screenRows - 1);
					leftTiles();
					bottomTiles();
				}
				else {
					// Move left
					--_startAddress;
					_bufferTopLeft -= 2;
					addUpdateBlock(0, 0, 1, _screenRows);
					leftTiles();
					topTiles();
					bottomTiles();
				}
			}
		}
		else {
			if (deltaX > 0) {
				if (deltaY < 0) {
					// Move up and right
					_startAddress -= _screenColumns - 1;
					_bufferTopLeft -= _screenWidthBytes - 2;
					addUpdateBlock(0, 0, _screenColumns, 1);
					addUpdateBlock(_screenColumns - 1, 1, 1, _screenRows - 1);
					topTiles();
					rightTiles();
				}
				else {
					if (deltaY > 0) { 
						// Move down and right
						_startAddress += _screenColumns + 1;
						_bufferTopLeft += _screenWidthBytes + 2;
						addUpdateBlock(0, _screenRows - 1, _screenColumns, 1);
						addUpdateBlock(_screenColumns - 1, 0, 1,
							_screenRows - 1);
						bottomTiles();
						rightTiles();
					}
					else {
						// Move right
						++_startAddress;
						_bufferTopLeft += 2;
						addUpdateBlock(_screenColumns - 1, 0, 1, _screenRows);
						rightTiles();
						topTiles();
						bottomTiles();
					}
				}
			}
			else {
				if (deltaY < 0) {
					// Move up
					_startAddress -= _screenColumns;
					_bufferTopLeft -= _screenWidthBytes;
					addUpdateBlock(0, 0, _screenColumns, 1);
					topTiles();
				}
				else {
					if (deltaY > 0) {
						// Move down
						_startAddress += _screenColumns;
						_bufferTopLeft += _screenWidthBytes;
						addUpdateBlock(0, _screenRows - 1, _screenColumns, 1);
						bottomTiles();
					}
					else {
						// No change
					}
				}
			}
		}





        // Time: inactive region starts
        // IRQ0 fires to start VRAM update

        // offScreenHandler
        // EOI
        // Switch IRQ0 handler to onScreenHandler
        // Set count for PIT channel 0 to active cycles
        // Start background sound
        // Start interrupts
        // Switch stack to VRAM update list
        // Copy from buffer to VRAM
        for (auto i : _updateBlocks)
            updateBlock(i);
		_updateBlocks.clear();
    }
    bool keyboardEvent(int key, bool up)
    {
        switch (key) {
            case VK_RIGHT:
                _rightPressed = !up;
                return true;
            case VK_LEFT:
                _leftPressed = !up;
                return true;
            case VK_UP:
                _upPressed = !up;
                return true;
            case VK_DOWN:
                _downPressed = !up;
                return true;
            case VK_SPACE:
                _spacePressed = !up;
                return true;
        }
        return false;
    }
private:
    struct Node
    {
        Byte drawn;
        Byte undrawn;
        Node* next;
		Node* previous;
    };
    struct UpdateBlock
    {
        Word bufferTopLeft;
        Word vramTopLeft;
        Word sourceAdd;
        Word destinationAdd;
        Word columns;
        Word rows;
    };

	void drawTile(Word tl, Word map)
	{
		tl += _bufferTL;
		map += _mapTL;
		const Byte* f = &_tiles[_tilePointers[_foreground[map]]];
		const Byte* b = &_tiles[_tilePointers[_background[map]]];
		int rowIncrement = _bufferStride - _tileWidthBytes;
		Byte* buffer = &_buffer[0];
		for (int y = 0; y < _tileRows; ++y) {
			for (int x = 0; x < _tileColumns; ++x) {
				Word c = *reinterpret_cast<const Word*>(f);
				Word* p = reinterpret_cast<Word*>(buffer + tl);
				if (c != 0xffff)
					*p = c;
				else
					*p = *reinterpret_cast<const Word*>(b);
				f += 2;
				b += 2;
				tl += 2;
			}
			tl += rowIncrement;
		}
	}

    void updateBlock(UpdateBlock b)
    {
        Word s = b.bufferTopLeft;
        Word d = b.vramTopLeft;
        Byte* buffer = &_buffer[0];
        for (int y = 0; y < b.rows; ++y) {
            for (int x = 0; x < b.columns; ++x) {
                *reinterpret_cast<Word*>(_vram + d) =
                    *reinterpret_cast<Word*>(buffer + s);
                s += 2;
                d += 2;
            }
            s += b.sourceAdd;
            d += b.destinationAdd;
        }
    }
	void addUpdateBlock(int left, int top, int columns, int rows)
	{
		UpdateBlock b;
		b.bufferTopLeft = left + top*_bufferStride + _bufferTopLeft;
		b.vramTopLeft = left + top*_screenWidthBytes + _startAddress;
		b.sourceAdd = _bufferStride - columns*2;
		b.destinationAdd = _screenWidthBytes - columns*2;
		b.columns = columns;
		b.rows = rows;
		_updateBlocks.append(b);
	}
	void removeNodeFromList(Node* node)
	{
		Node* next = node->next;
		Node* previous = node->previous;
		next->previous = previous;
		previous->next = next;
	}
	void insertNodeAfter(Node* node, Node* newPrevious)
	{
		Node* newNext = newPrevious->next;
		node->previous = newPrevious;
		node->next = newNext;
		newPrevious->next = node;
		newNext->previous = node;
	}
	Node* getNode()
	{
		Node* node = _freeNodes.next;
		removeNodeFromList(node);
		return node;
	}
	void freeNode(Node* node)
	{
		insertNodeAfter(node, &_freeNodes);
	}
	void initList(Node* node)
	{
		node->next = node;
		node->previous = node;
	}
	void shiftLeft(Node* list)
	{
		Node* n = list->next;
		if (n->drawn != 0) {
			--n->drawn;
			return;
		}
		if (n->undrawn != 0) {
			--n->undrawn;
			if (n->undrawn == 0) {
				removeNodeFromList(n);
				freeNode(n);
			}
		}
	}
	void shiftUp(Node* list)
	{
		Node* n = list->next;
		if (n->drawn != 0) {
			--n->drawn;
			return;
		}
		if (n->undrawn != 0) {
			--n->undrawn;
			if (n->undrawn == 0) {
				removeNodeFromList(n);
				freeNode(n);
			}
		}
	}
	void shiftRight(Node* list)
	{
	}
	void shiftDown(Node* list)
	{
	}
	void clearList(Node* list)
	{
		Node* n = list->next;
		Node* p = list->previous;
		if (n != p) {
			Node* nn = n->next;
			nn->previous = &_freeNodes;
			p->next = _freeNodes.next;
			_freeNodes.next->previous = p;
			_freeNodes.next = nn;

			_freeNodes.previous = p;
			_freeNodes.next = nn;

			n->next = list;
			list->previous = n;
		}
	}
	void setListEmpty(Node* list, int tiles)
	{
		list->drawn = 0;
		list->next->drawn = 0;
		list->next->undrawn = tiles;
		clearList(list);
	}
	void setListFilled(Node* list, int tiles)
	{
		list->drawn = tiles;
		list->next->drawn = tiles;
		list->next->undrawn = 0;
		clearList(list);
	}
	int findUndrawnTile(Node* list)
	{
		Node* n = list->next;
		int i = n->drawn;
		++n->drawn;
		--n->undrawn;
		if (n->undrawn == 0) {
			Node* nn = n->next;
			if (nn != list) {
                n->drawn += nn->drawn;
			    n->undrawn = nn->undrawn;
			    removeNodeFromList(n);
			    freeNode(n);
			}
		}
		return i;
	}
	void leftTiles()
	{
		while (_leftNodes.drawn < _transitionCountLeft[_xSubTile >> 8]) {
			int y = findUndrawnTile(&_leftNodes);
			drawTile(_bufferLeft[y], _mapLeft[y]);
		}
	}
	void topTiles()
	{
		while (_topNodes.drawn < _transitionCountTop[_ySubTile >> 8]) {
			int x = findUndrawnTile(&_topNodes);
			drawTile(_bufferTop[x], x);
		}
	}
	void rightTiles()
	{
		while (_rightNodes.drawn < _transitionCountRight[_xSubTile >> 8]) {
			int y = findUndrawnTile(&_rightNodes);
			drawTile(_bufferRight[y], _mapRight[y]);
		}
	}
	void bottomTiles()
	{
		while (_bottomNodes.drawn < _transitionCountBottom[_ySubTile >> 8]) {
			int x = findUndrawnTile(&_bottomNodes);
			drawTile(_bufferBottom[x], _mapBottom[x]);
		}
	}


    FFTWWisdom<float> _wisdom;
    CGAData _data;
    CGASequencer _sequencer;
    CGAOutput _output;
    AnimatedWindow _animated;
    BitmapWindow _bitmap;
	Vector _outputSize;
	Array<Byte> _cgaBytes;
	Byte* _vram;
	int _regs;

    Array<Byte> _background;
    Array<Byte> _foreground;
    Array<Byte> _buffer;
    Array<Byte> _tiles;

    int _tileColumns;
    int _tileWidthBytes;
    int _tileRows;
    int _bufferStride;
    int _screenColumns;
	int _screenWidthBytes;
    int _screenRows;
    int _mapStride;
	int _tilesPerScreenHorizontally;
	int _tilesPerScreenVertically;
    int _horizontalAcceleration;
    int _verticalAcceleration;
    int _horizontalMaxVelocity;
    int _verticalMaxVelocity;
	Array<Byte> _transitionCountLeft;
	Array<Byte> _transitionCountTop;
	Array<Byte> _transitionCountRight;
	Array<Byte> _transitionCountBottom;
	Array<Word> _bufferLeft;
	Array<Word> _mapLeft;
	Array<Word> _bufferTop;
	Array<Word> _bufferRight;
	Array<Word> _mapRight;
	Array<Word> _bufferBottom;
	Array<Word> _mapBottom;
	Array<Word> _tilePointers;

    bool _upPressed;
    bool _downPressed;
    bool _leftPressed;
    bool _rightPressed;
    bool _spacePressed;

    int _xVelocity;
    int _yVelocity;
    int _xSubTile;
    int _ySubTile;
	Word _startAddress;
	Word _bufferTopLeft;  // Position in buffer corresponding to TL of screen
	Word _bufferTL; // Position in buffer corresponding to TL of tilescreen
	Word _mapTL;    // Position in map corresponding to TL of tilescreen 

    AppendableArray<UpdateBlock> _updateBlocks;
	Array<Node> _nodePool;
	Node* _nodes;
	Node _freeNodes;
	Node _leftNodes;
	Node _topNodes;
	Node _rightNodes;
	Node _bottomNodes;
};

class Program : public WindowProgram<GameWindow>
{
};
