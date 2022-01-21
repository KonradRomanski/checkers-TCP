import { useCheckers } from 'react-checkers';
import {useState} from "react";

function Pawn(props) {
    return <div onClick={props.onClick}
                style={{
                    width: "20px",
                    height: "20px",
                    borderRadius: "100%",
                    backgroundColor: props.square.occupiedBy == 1 ? "pink" : "lightblue"
                }}>
        {props.square.isKinged && "K"}
    </div>;
}

function Square(props) {
    const {onClick, evenPosition, square} = props;
    return <div onClick={props.onClick}
                style={{width: "30px", height: "30px", backgroundColor: props.evenPosition ? "grey" : "black"}}>
        {props.square.occupiedBy !== null && props.evenPosition && (
            <Pawn onClick={props.onClick1} square={props.square}/>
        )}
    </div>;
}

export function Checkers() {
    const { board, handleMove, handlePick, playerTurn, scoreboard, rules } = useCheckers(10);
    const [activeSquare, setActiveSquare] = useState();

    function customHandlePick(square) {
        setActiveSquare(square);
        handlePick(square);
    }

    function customHandleMove(square) {
        console.log({from: activeSquare, to: square});
        setActiveSquare(null);
        return handleMove(square);
    }

    return (
        <div>
            <div>Turn: {playerTurn}</div>
            <div>
                <div>Score:</div>
                <div>Player 1: {scoreboard[1]} | Player 2: {scoreboard[2]}</div>
            </div>
            <div>
                {Object.keys(board).map((row, j) => {
                    return (
                        <div key={j} style={{display: 'flex', 'flex-direction': 'row'}}>
                            {Object.keys(board[row]).map((positionIndex, k) => {
                                const square = board[row][positionIndex];
                                const isEvenPosition = (square.position.x + 1 * square.position.y) % 2 === 0
                                return (
                                    <Square onClick={() => (!square.occupiedBy ? customHandleMove(square) : null)}
                                            evenPosition={isEvenPosition} square={square}
                                            onClick1={() => customHandlePick(square)}/>
                                )
                            })}
                        </div>
                    )
                })}
            </div>
            <div>
                <div>Rules:</div>
                <div>
                    {rules.map((rule, i) => (
                        <div key={i}>
                            {i + 1}. {rule}
                        </div>
                    ))}
                </div>
            </div>
        </div>
    );
}
