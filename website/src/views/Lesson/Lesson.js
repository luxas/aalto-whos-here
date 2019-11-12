import React from "react";
// @material-ui/core components
import { makeStyles } from "@material-ui/core/styles";
// core components
import GridItem from "components/Grid/GridItem.js";
import GridContainer from "components/Grid/GridContainer.js";
import Table from "components/Table/Table.js";
import Card from "components/Card/Card.js";
import CardHeader from "components/Card/CardHeader.js";
import CardBody from "components/Card/CardBody.js";
import Tasks from "components/Tasks/Tasks.js";
import CardFooter from "components/Card/CardFooter.js";
import UpdateIcon from "@material-ui/icons/Update";
import CardIcon from "components/Card/CardIcon.js";
import Accessibility from "@material-ui/icons/Accessibility";
import importedstyles from "assets/jss/material-dashboard-react/views/dashboardStyle.js";


const attendees = [
    "Lucas Käldström",
    "Veera Ihalainen",
    "Sophie Truong",
]

const styles = Object.assign(importedstyles, {
  cardCategoryWhite: {
    "&,& a,& a:hover,& a:focus": {
      color: "rgba(255,255,255,.62)",
      margin: "0",
      fontSize: "14px",
      marginTop: "0",
      marginBottom: "0"
    },
    "& a,& a:hover,& a:focus": {
      color: "#FFFFFF"
    }
  },
  cardTitleWhite: {
    color: "#FFFFFF",
    marginTop: "0px",
    minHeight: "auto",
    fontWeight: "300",
    fontFamily: "'Roboto', 'Helvetica', 'Arial', sans-serif",
    marginBottom: "3px",
    textDecoration: "none",
    "& small": {
      color: "#777",
      fontSize: "65%",
      fontWeight: "400",
      lineHeight: "1"
    }
  }
});

const useStyles = makeStyles(styles);

export default function TableList() {
  const classes = useStyles();
  return (
      <div>
          <GridContainer>
            <GridItem xs={12} sm={6} md={3}>
            <Card>
                <CardHeader color="info" stats icon>
                <CardIcon color="info">
                <Accessibility />
                </CardIcon>
                <p className={classes.cardCategory}>Followers</p>
                <h3 className={classes.cardTitle}>+245</h3>
                </CardHeader>
                <CardFooter stats>
                <div className={classes.stats}>
                    <UpdateIcon />
                    Just Updated
                </div>
                </CardFooter>
            </Card>
            </GridItem>
        </GridContainer>
        <GridContainer>
      <GridItem xs={12} sm={12} md={12}>
        <Card>
          <CardHeader color="primary">
            <h4 className={classes.cardTitleWhite}>Lesson 05.11.2019</h4>
            <p className={classes.cardCategoryWhite}>
              Lesson description
            </p>
          </CardHeader>
          <CardBody>
          <Tasks
            checkedIndexes={[0, 2]}
            tasksIndexes={[0, 1, 2]}
            tasks={attendees} />
          </CardBody>
        </Card>
      </GridItem>
    </GridContainer>
      </div>
  );
}
